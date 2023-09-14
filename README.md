# Aeroh Link Firmware

## How to do Firmware Upgrade

Make and commit all your changes, and then run this command to release.

```bash
python3 scripts/release_firmware.py
```

Also, update `app/resources/api/v1/device_resource.rb` in the `aeroh-cloud` repo.

## How update OTA update certificate

Run this while you are in the root directory of this repository

```bash
echo -n | openssl s_client -showcerts -connect ota.aeroh.org:443 2>/dev/null | sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' > certificate.crt
xxd -i certificate.crt > components/cloud/include/ota_certificate.h
rm certificate.crt
```
