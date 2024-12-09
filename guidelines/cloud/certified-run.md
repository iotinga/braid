![Finanziato dall'Unione europea | Ministero dell'Università e della Ricerca | Italia domani PNRR | iNEST ](../../assets/HEADER_INEST.png)

# Certified running broker software

> BRAID connected sensors boards (a.k.a. devices or agents) have to apply a signature upon each message sent to the broker as stated in the MQTT protocol definiton. Private keys are securely stored on each device and no-one outside the devices never knows them. This is a important achievement, so we can "certify" that sensors data records are genuine using the well known Private / Public keys mechanisms. 
> This is not enough for us because, as developers and producers of the devices, we may be able to produce agents with tweakable system clocks and let the device sign values with a wrong timestamp. As we want do be able to prove data integrity in any case, the solution is to use an open-source broker, running on a third party system, that shall apply a signed time mark to each message. In this way we're able to validate not only the data source but also the collection timestamp of a sensor read.

# Guidelines for Deploying a Verifiable MQTT Broker (Mosquitto)

## Introduction

This document provides guidelines for deploying a verifiable instance of the Mosquitto MQTT broker. These steps integrate concepts from **Reproducible Builds** and **Remote Attestation** to ensure the running instance is compliant with its source code.

## Prerequisites

1. **Environment Setup**:
   - Linux-based operating system with Docker installed.
   - TPM (Trusted Platform Module) or Intel SGX-enabled hardware for remote attestation (optional but recommended).

2. **Dependencies**:
   - Mosquitto source code from the [official repository](https://github.com/eclipse/mosquitto).
   - Build tools: `gcc`, `make`, `cmake`.

3. **Cryptographic Tools**:
   - GPG for signing the source and binaries.
   - Hashing tools such as `sha256sum`.

## Steps for Verifiable Deployment

### 1. **Clone and Verify the Source Code**
Clone the official Mosquitto repository and verify its integrity:
```bash
git clone https://github.com/eclipse/mosquitto.git
cd mosquitto
git verify-tag <release_tag>
```

### 2. **Build Reproducible Binaries**
Ensure the build process is reproducible:
- Use Docker to define a reproducible build environment.

#### Dockerfile Example:
```dockerfile
FROM ubuntu:20.04
RUN apt-get update && apt-get install -y build-essential cmake git
COPY . /mosquitto
WORKDIR /mosquitto
RUN cmake . && make
```

#### Build the Docker Image:
```bash
docker build -t mosquitto-reproducible .
docker run --rm mosquitto-reproducible sha256sum mosquitto
```

Save the resulting checksum for verification.

### 3. **Sign the Source and Binary**
Sign both the source code and the resulting binary:
```bash
gpg --output mosquitto-source.sig --sign --detach mosquitto-source.tar.gz
gpg --output mosquitto-binary.sig --sign --detach mosquitto
```

Publish these signatures and checksums on a trusted server or repository.

### 4. **Configure the Broker for Verification**
Set up Mosquitto to expose its binary hash and build metadata via an endpoint:
1. Modify the Mosquitto configuration to include a verification API.
2. Example of a simple verification endpoint using `HTTP`:
   ```json
   {
       "binary_hash": "abc123...xyz",
       "source_commit": "4d5e6f7g...",
       "build_date": "2024-12-09"
   }
   ```

### 5. **Implement Remote Attestation**
Use TPM or SGX for runtime attestation:
1. Install and configure a remote attestation agent (e.g., Intel SGX SDK or TPM tools).
2. The attestation process should verify:
   - Binary integrity (matches the signed checksum).
   - Secure configuration of the runtime environment.
   - Timestamps to ensure non-repudiation.

### 6. **Deployment**
Deploy the verified Mosquitto instance:
```bash
docker run -d --name verified-mosquitto mosquitto-reproducible
```

## Verification Process

1. **Reproduce the Build**:
   - Any party can clone the Mosquitto repository, build the binaries using the same Docker environment, and verify the checksum.

2. **Check Binary Integrity**:
   - Compare the running instance's binary hash with the published checksum.

3. **Perform Remote Attestation**:
   - Verify the running instance using remote attestation protocols to ensure its integrity and compliance with the expected configuration.

## Additional Resources

- [Reproducible Builds Project](https://reproducible-builds.org/)
- [Eclipse Mosquitto](https://mosquitto.org/)
- [Intel SGX Developer Guide](https://software.intel.com/content/www/us/en/develop/topics/software-guard-extensions.html)
- [TPM Tools](https://tpm2-software.github.io/)

## Conclusion

By following these steps, you can deploy a Mosquitto MQTT broker instance that is verifiable by third parties, ensuring compliance with the source code and enhancing trust in your system.

This markdown document provides a detailed roadmap for deploying a verifiable Mosquitto MQTT broker while integrating reproducibility and remote attestation techniques.


# Bibliography

[1] Lamb, C., & Zacchiroli, S. (2021). Reproducible Builds: Increasing the Integrity of Software Supply Chains. IEEE Software, 0–0. doi:10.1109/ms.2021.3073045 

[2] Alexander Sprogø Banks, Marek Kisiel, Philip Korsholm (2021). Remote Attestation: A Literature Review