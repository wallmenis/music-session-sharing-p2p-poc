
#include <iostream>
#include <string>
#include <openssl/evp.h>

int main()
{
    
    return 0;
}

std::string sha256sum(const std::string input)
{
    unsigned char hash[EVP_MAX_SHA256_SIZE];
    EVP_sha256 *shasum;
    //EVP_DigestInit_ex
    return 
}