/*
 *
 * Copyright (c) 2022 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#if defined(SGX_RA_TLS_TDX_BACKEND) || defined (SGX_RA_TLS_AZURE_TDX_BACKEND)

#include <grpcpp/security/sgx/sgx_ra_tls_backends.h>
#include <grpcpp/security/sgx/sgx_ra_tls_impl.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <assert.h>
#include <fstream>
#include <cstring>

#ifdef SGX_RA_TLS_AZURE_TDX_BACKEND
#include <azguestattestation1/AttestationClient.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include "azure_tdx/Utils.h"
#include "azure_tdx/Logger.h"
#include "azure_tdx/AttestClient.h"
#include "azure_tdx/HttpClient.h"
#endif

#ifdef SGX_RA_TLS_TDX_BACKEND
#include "sgx_urts.h"
#endif

namespace grpc {
namespace sgx {

#include <tdx_attest.h>

#ifdef SGX_RA_TLS_TDX_BACKEND
#include <sgx_ql_quote.h>
#include <sgx_dcap_quoteverify.h>
#endif

#ifdef SGX_RA_TLS_AZURE_TDX_BACKEND
using json = nlohmann::json;
using namespace std;
using namespace std::chrono;
#endif

const uint8_t g_att_key_id_list[256] = {0};

static void tdx_gen_report_data(uint8_t *reportdata) {
    srand(time(NULL));
    for (int i = 0; i < TDX_REPORT_DATA_SIZE; i++) {
        reportdata[i] = rand();
    }
}

int tdx_parse_quote(X509 *x509, uint8_t **quote, uint32_t &quote_size) {
    return parse_quote(x509, quote, quote_size);
};

void tdx_verify_init() {
    generate_key_cert(dummy_generate_quote);
};

ra_tls_measurement tdx_parse_measurement(const char *der_crt, size_t len) {
    // TODO
    return ra_tls_measurement();
}

#ifdef SGX_RA_TLS_AZURE_TDX_BACKEND
// input:  uint8_t *hash
// output: uint8_t **quote_buf
// output: uint32_t &quote_size
static int tdx_generate_quote(
        uint8_t **quote_buf, uint32_t &quote_size, uint8_t *hash) {

  int ret = -1;

  try {
    AttestationClient *attestation_client = nullptr;
    Logger *log_handle = new Logger();

    // Initialize attestation client
    if (!Initialize(log_handle, &attestation_client)) {
      grpc_fprintf(stderr, "Error: Failed to create attestation client object\n\n");
      Uninitialize();
      return(0);
    }
    attest::AttestationResult result;

    auto start = high_resolution_clock::now();

    // TODO: When GetHardwarePlatformEvidence supports a report data input parameter,
    //       pass the public key hash to be used as REPORTDATA. This hash is used by
    //       the verifying party to verify the TD that created generated the X.509 cert
    //       and associated public key, is the same TD that generated the TD report/quote.
    grpc_printf("Warning: Adding public key hash to TD report data is currently not supported.\n");
    unsigned char *evidence = nullptr;
    result = attestation_client->GetHardwarePlatformEvidence(&evidence);

    auto stop = high_resolution_clock::now();
    duration<double, std::milli> elapsed = stop - start;

    if (result.code_ != attest::AttestationResult::ErrorCode::SUCCESS) {
      grpc_fprintf(stderr, "Error: Failed to get quote\n\n");
      Uninitialize();
      return(0);
    }

    std::string quote_data;
    quote_data = reinterpret_cast<char *>(evidence);

    // Parses the returned json response
    json json_response = json::parse(quote_data);

    std::string encoded_quote = json_response["quote"];
    if (encoded_quote.empty()) {
      result.code_ = attest::AttestationResult::ErrorCode::ERROR_EMPTY_TD_QUOTE;
      result.description_ = std::string("Error: Empty Quote received from IMDS Quote Endpoint");
      Uninitialize();
      return(0);
    }

    // decode the base64url encoded quote to raw bytes
    std::vector<unsigned char> quote_bytes = Utils::base64url_to_binary(encoded_quote);

    quote_size = quote_bytes.size();
    *quote_buf = (uint8_t *)realloc(*quote_buf, quote_size);
    memcpy(*quote_buf, (uint8_t *)quote_bytes.data(), quote_size);

    print_hex_dump("Info: TDX quote data\n", " ", *quote_buf, quote_size);

    Uninitialize();
  }
  catch (std::exception &e) {
    cout << "Error: Exception occured. Details - " << e.what() << endl;
    return(0);
  }

  return ret;
};

// input:  uint8_t *quote_buf
// input:  size_t quote_size
// output: uint8_t **hash 
int tdx_verify_quote(uint8_t *quote_buf, size_t quote_size, uint8_t **hash_buf) {
  int ret = -1;

  try {
    std::string config_filename = "/etc/azure_tdx_config.json";

    // set attestation request based on config file
    std::ifstream config_file(config_filename);
    json config;
    if (config_file.is_open()) {
      config = json::parse(config_file);
      config_file.close();
    } else {
        grpc_fprintf(stderr, "Error: Failed to open config file\n\n");
        return(ret);
    }

    std::string attestation_url;
    if (!config.contains("attestation_url")) {
      grpc_fprintf(stderr, "Error: Attestation_url is missing\n\n");
      return(ret);
    }
    attestation_url = config["attestation_url"];

    std::string api_key;
    if (config.contains("api_key")) {
      api_key = config["api_key"];
    }

    bool metrics_enabled = false;
    if (config.contains("enable_metrics")) {
      metrics_enabled = config["enable_metrics"];
    }

    std::string provider;
    if (!config.contains("attestation_provider")) {
      grpc_fprintf(stderr, "Error: Attestation_provider is missing\n\n");
      return(ret);
    }
    provider = config["attestation_provider"];

    if (!Utils::case_insensitive_compare(provider, "amber") &&
        !Utils::case_insensitive_compare(provider, "maa")) {
      grpc_fprintf(stderr, "Erro: Attestation provider is invalid\n\n");
      return(ret);
    }

    std::map<std::string, std::string> hash_type;
    hash_type["maa"] = "sha256";
    hash_type["amber"] = "sha512";

    // check for user claims
    std::string client_payload;
    json user_claims = config["claims"];
    if (!user_claims.is_null()) {
      client_payload = user_claims.dump();
    }

    // if attesting with Amber, we need to make sure an API token was provided
    if (api_key.empty() && Utils::case_insensitive_compare(provider, "amber")) {
      grpc_fprintf(stderr, "Error: Attestation endpoint \"api_key\" value missing\n\n");
      return(ret);
    }

    print_hex_dump("Info: Received TDX quote data\n", " ", quote_buf, quote_size);

    std::vector<unsigned char> quote_vector(quote_buf, quote_buf + quote_size);
    std::string encoded_quote = Utils::binary_to_base64url(quote_vector);

    // For now, pass empty claim
    std::string json_claims = "{}";
    std::vector<unsigned char> claims_vector(json_claims.begin(), json_claims.end());
    std::string encoded_claims = Utils::binary_to_base64url(claims_vector);

    HttpClient http_client;
    AttestClient::Config attestation_config = {
        attestation_url,
        provider,
        encoded_quote,
        encoded_claims,
        api_key};

    auto start = high_resolution_clock::now();
    std::string jwt_token = AttestClient::VerifyEvidence(attestation_config, http_client);
    auto stop = high_resolution_clock::now();
    duration<double, std::milli> token_elapsed = stop - start;

    if (jwt_token.empty()) {
      fprintf(stderr, "Error: Empty token received\n");
      return(ret);
    }

    // Parse TEE-specific claims from JSON Web Token
    std::vector<std::string> tokens;
    boost::split(tokens, jwt_token, [](char c) {return c == '.'; });
    if (tokens.size() < 3) {
      fprintf(stderr, "Error: Invalid JWT token\n");
      return(ret);
    }

    json attestation_claims = json::parse(Utils::base64_decode(tokens[1]));
    try {
        std::string tdx_report_data;
        if (Utils::case_insensitive_compare(provider, "maa"))
            tdx_report_data = attestation_claims["tdx_report_data"].get<std::string>();
        else if (Utils::case_insensitive_compare(provider, "amber"))
            tdx_report_data = attestation_claims["amber_report_data"].get<std::string>();

	// Return the public key hash, which is the first 32 bytes of the report data.
	std::vector<unsigned char> hash_vector(tdx_report_data.begin(), tdx_report_data.end());
        *hash_buf = (uint8_t *)realloc(*hash_buf, SHA256_DIGEST_LENGTH);
        memcpy(*hash_buf, (uint8_t *)hash_vector.data(), SHA256_DIGEST_LENGTH);
        print_hex_dump("Info: Public key hash from report data\n", " ", *hash_buf, SHA256_DIGEST_LENGTH);
    }
    catch (...) {
        fprintf(stderr, "Error: JWT missing TD report custom data\n");
        return(ret);
    }

    grpc_printf("Info: Quote verification completed successfully.\n");

    return(0);
  }
  catch (std::exception &e) {
    cout << "Error: Exception occured. Details - " << e.what() << endl;
    return(1);
  }
}

// input: const char *der_crt
// input: size_t len
int tdx_verify_cert(const char *der_crt, size_t len) {
    int ret = 0;
    uint32_t quote_size = 0;
    uint8_t *quote_buf = nullptr;
    uint8_t *hash_buf = nullptr;    

    BIO *bio = BIO_new(BIO_s_mem());
    BIO_write(bio, der_crt, len);
    X509 *x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL);
    if (!x509) {
        grpc_printf("Error: Failed to parse certificate.\n");
        goto out;
    }

    ret = tdx_parse_quote(x509, &quote_buf, quote_size);
    if (ret != 0) {
        grpc_printf("Error: Failed to parse quote.\n");
        goto out;
    }

    ret = tdx_verify_quote(quote_buf, quote_size, &hash_buf);
    if (ret != 0) {
        grpc_printf("Error: Failed to verify quote.\n");
        goto out;
    }

    // TODO: Enable when GetHardwarePlatformEvidence supports adding
    //       REPORTDATA to the TD report.
#if 0
    ret = verify_pubkey_hash(x509, hash_buf, SHA256_DIGEST_LENGTH);
    if (ret != 0) {
        grpc_printf("Error: Failed to verify public key hash.\n");
        goto out;
    }
#else
	grpc_printf("Warning: Verification of pulic key hash is disabled.\n");
#endif

    // ret = verify_measurement((const char *)&p_rep_body->mr_enclave,
    //                          (const char *)&p_rep_body->mr_signer,
    //                          (const char *)&p_rep_body->isv_prod_id,
    //                          (const char *)&p_rep_body->isv_svn);

out:
    BIO_free(bio);
    return ret;
}
#endif // SGX_RA_TLS_AZURE_TDX_BACKEND

#ifdef SGX_RA_TLS_TDX_BACKEND
static int tdx_generate_quote(
        uint8_t **quote_buf, uint32_t &quote_size, uint8_t *hash) {
    int ret = -1;

    tdx_report_data_t report_data = {{0}};
    tdx_report_t tdx_report = {{0}};
    tdx_uuid_t selected_att_key_id = {0};

    tdx_gen_report_data(report_data.d);
    // print_hex_dump("TDX report data\n", " ", report_data.d, sizeof(report_data.d));

    if (TDX_ATTEST_SUCCESS != tdx_att_get_report(&report_data, &tdx_report)) {
        grpc_fprintf(stderr, "failed to get the report.\n");
        ret = 0;
    }
    // print_hex_dump("TDX report\n", " ", tdx_report.d, sizeof(tdx_report.d));

    if (TDX_ATTEST_SUCCESS != tdx_att_get_quote(&report_data, NULL, 0, &selected_att_key_id,
        quote_buf, &quote_size, 0)) {
        grpc_fprintf(stderr, "failed to get the quote.\n");
        ret = 0;
    }
    // print_hex_dump("TDX quote data\n", " ", *quote_buf, quote_size);

    // printf("tdx_generate_quote, sizeof %d, quote_size %d\n", sizeof(*quote_buf), quote_size);

    realloc(*quote_buf, quote_size+SHA256_DIGEST_LENGTH);
    memcpy((*quote_buf)+quote_size, hash, SHA256_DIGEST_LENGTH);
    quote_size += SHA256_DIGEST_LENGTH;

    // printf("tdx_generate_quote, sizeof %d, quote_size %d\n", sizeof(*quote_buf), quote_size);
    return ret;
};

int tdx_verify_quote(uint8_t *quote_buf, size_t quote_size) {
    bool use_qve = false;
    (void)(use_qve);

    int ret = 0;
    time_t current_time = 0;
    uint32_t supplemental_data_size = 0;
    uint8_t *p_supplemental_data = nullptr;

    quote3_error_t dcap_ret = SGX_QL_ERROR_UNEXPECTED;
    sgx_ql_qv_result_t quote_verification_result = SGX_QL_QV_RESULT_UNSPECIFIED;
    uint32_t collateral_expiration_status = 1;

    sgx_status_t sgx_ret = SGX_SUCCESS;
    uint8_t rand_nonce[16] = "59jslk201fgjmm;";
    sgx_ql_qe_report_info_t qve_report_info;
    sgx_launch_token_t token = { 0 };

    int updated = 0;
    quote3_error_t verify_qveid_ret = SGX_QL_ERROR_UNEXPECTED;
    sgx_enclave_id_t eid = 0;

    // call DCAP quote verify library to get supplemental data size
    dcap_ret = tdx_qv_get_quote_supplemental_data_size(&supplemental_data_size);
    if (dcap_ret == SGX_QL_SUCCESS && \
        supplemental_data_size == sizeof(sgx_ql_qv_supplemental_t)) {
        grpc_printf("Info: tdx_qv_get_quote_supplemental_data_size successfully returned.\n");
        p_supplemental_data = (uint8_t*)malloc(supplemental_data_size);
    } else {
        grpc_printf("Error: tdx_qv_get_quote_supplemental_data_size failed: 0x%04x\n", dcap_ret);
        supplemental_data_size = 0;
    }

    // set current time. This is only for sample purposes, in production mode a trusted time should be used.
    current_time = time(NULL);

    // call DCAP quote verify library for quote verification
    print_hex_dump("TDX parse quote data\n", " ", quote_buf, quote_size);
    dcap_ret = tdx_qv_verify_quote(
            quote_buf, quote_size,
            NULL,
            current_time,
            &collateral_expiration_status,
            &quote_verification_result,
            NULL,
            supplemental_data_size,
            p_supplemental_data);
    if (dcap_ret == SGX_QL_SUCCESS) {
        grpc_printf("Info: App: tdx_qv_verify_quote successfully returned.\n");
    } else {
        grpc_printf("Error: App: tdx_qv_verify_quote failed: 0x%04x\n", dcap_ret);
    }

    //check verification result
    switch (quote_verification_result) {
        case SGX_QL_QV_RESULT_OK:
            //check verification collateral expiration status
            //this value should be considered in your own attestation/verification policy
            //
            if (collateral_expiration_status == 0) {
                grpc_printf("Info: App: Verification completed successfully.\n");
                ret = 0;
            } else {
                grpc_printf("Warning: App: Verification completed, but collateral is out of date based on 'expiration_check_date' you provided.\n");
                ret = 1;
            }
            break;
        case SGX_QL_QV_RESULT_CONFIG_NEEDED:
        case SGX_QL_QV_RESULT_OUT_OF_DATE:
        case SGX_QL_QV_RESULT_OUT_OF_DATE_CONFIG_NEEDED:
        case SGX_QL_QV_RESULT_SW_HARDENING_NEEDED:
        case SGX_QL_QV_RESULT_CONFIG_AND_SW_HARDENING_NEEDED:
            grpc_printf("Warning: App: Verification completed with Non-terminal result: %x\n", quote_verification_result);
            ret = 1;
            break;
        case SGX_QL_QV_RESULT_INVALID_SIGNATURE:
        case SGX_QL_QV_RESULT_REVOKED:
        case SGX_QL_QV_RESULT_UNSPECIFIED:
        default:
            grpc_printf("Error: App: Verification completed with Terminal result: %x\n", quote_verification_result);
            ret = -1;
            break;
    }

    return ret;
}

int tdx_verify_cert(const char *der_crt, size_t len) {
    int ret = 0;
    uint32_t quote_size = 0;
    uint8_t *quote_buf = nullptr;

    BIO *bio = BIO_new(BIO_s_mem());
    BIO_write(bio, der_crt, len);
    X509 *x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL);
    if (!x509) {
        grpc_printf("parse the crt failed.\n");
        goto out;
    }

    ret = tdx_parse_quote(x509, &quote_buf, quote_size);
    if (ret != 0) {
        grpc_printf("parse quote failed.\n");
        goto out;
    }

    ret = tdx_verify_quote(quote_buf, quote_size-SHA256_DIGEST_LENGTH);
    if (ret != 0) {
        grpc_printf("verify quote failed.\n");
        goto out;
    }

    ret = verify_pubkey_hash(x509, quote_buf+quote_size-SHA256_DIGEST_LENGTH, SHA256_DIGEST_LENGTH);
    if (ret != 0) {
        grpc_printf("verify the public key hash failed.\n");
        goto out;
    }

    // ret = verify_measurement((const char *)&p_rep_body->mr_enclave,
    //                          (const char *)&p_rep_body->mr_signer,
    //                          (const char *)&p_rep_body->isv_prod_id,
    //                          (const char *)&p_rep_body->isv_svn);

out:
    BIO_free(bio);
    return ret;
}
#endif // SGX_RA_TLS_TDX_BACKEND

std::vector<std::string> tdx_generate_key_cert() {
    return generate_key_cert(tdx_generate_quote);
}

} // namespace sgx
} // namespace grpc

#endif // SGX_RA_TLS_TDX_BACKEND || SGX_RA_TLS_AZURE_TDX_BACKEND
