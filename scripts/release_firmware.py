import subprocess
import os

def run_command(cmd):
    return subprocess.check_output(cmd, shell=True)

def human_readable_size(size, decimal_places=2):
    for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
        if size < 1024.0:
            break
        size /= 1024.0
    return f"{size:.{decimal_places}f} {unit}"

def release_firmware():
    run_command("git fetch")
    current_latest_version = run_command("git tag | head").strip().decode("utf-8")
    firmwareVersion = FirmwareVersion(current_latest_version)

    print("Found the current latest version to be: " + str(firmwareVersion))
    option = input("Would you like to increment the major, minor, or patch version? \n[1] Major \n[2] Minor \n[3] Patch \n1,2,[3]:").strip()
    if option in ["3", ""]:
        print("Incrementing patch version")
        firmwareVersion.patch = str(int(firmwareVersion.patch) + 1)
    elif option == "2":
        print("Incrementing minor version")
        firmwareVersion.minor = str(int(firmwareVersion.minor) + 1)
    elif option == "1":
        print("Incrementing major version")
        firmwareVersion.major = str(int(firmwareVersion.major) + 1)
    else:
        print(f"Invalid option entered '{option}'. Enter 1,2,3. Exiting.")
        exit(1)

    print("New version is: " + str(firmwareVersion))

    option = input("Accept the new version? \n[y/Y] Yes \n[n/N] No \ny,[N]:").strip()

    if option in ["y", "Y"]:
        print("Checking current working directory is clean")
        if run_command("git status --porcelain"):
            print("Working directory is not clean. Exiting.")
            exit(1)

        print("Updating version")
        # ask for change log
        changelog = input("Enter changelog: \n").strip()

        # build firmware
        repo_directory = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        print("Repo directory: " + repo_directory)

        print("========> Step #1 <========")
        print("Updating version inside the firmware by updating sdkconfig file")
        sdkconfig = open("sdkconfig").read()
        sdkconfig = sdkconfig.replace(f"CONFIG_FIRMWARE_VERSION=\"{current_latest_version[1:]}\"", f"CONFIG_FIRMWARE_VERSION=\"{str(firmwareVersion)[1:]}\"")
        open("sdkconfig", "w").write(sdkconfig)
        print("Done")

        print("========> Step #2 <========")
        print("Building firmware...")
        run_command(f"cd {repo_directory}")
        run_command(f". ~/esp/esp-idf/export.sh")
        run_command(f"idf.py build")

        firmware_path = "build/aeroh-one-esp32c3-fw.bin"

        print("Firmware built at: " + firmware_path)
        firmware_size = os.path.getsize(repo_directory + "/" + firmware_path)
        print("Firmware size: " + human_readable_size(firmware_size))

        print("========> Step #3 <========")
        run_command(f"git add sdkconfig")
        run_command(f"git commit -m 'version bump to {str(firmwareVersion)}'")
        run_command(f"git push origin main")

        # upload to s3
        print("========> Step #4 <========")
        bucket_name = "aeroh-ota"
        file_name = f"aeroh-link-fw-{str(firmwareVersion)}.bin"
        bucket_path = f"link/{file_name}"
        base_url = "https://ota.aeroh.org/"
        print(f"Uploading firmware to s3 bucket: {bucket_name} at path: {bucket_path}")
        print(f"Firmware will be available at: {base_url}{bucket_path}")
        upload_link = "https://s3.console.aws.amazon.com/s3/upload/aeroh-ota?region=us-east-1&prefix=link/"
        run_command(f"cp {firmware_path} {file_name}")
        print(f"Since this script doesn't have access to AWS, please open this link: {upload_link}.\nWe have made the file available in the repo directory: {repo_directory}")

        # tag release
        print("========> Step #5 <========")
        additional_information = "\n\nPresence of this tag indicate that the firmware is available to download from ota.aeroh.org.\nFollow these instructions at https://aeroh.atlassian.net/wiki/spaces/HE/pages/11403265/How+To+Release+a+new+Firmware to understand how to release a firmware to users"
        run_command(f"git tag {firmwareVersion} -m \"Aeroh Link Firmware {firmwareVersion}\" -m \"{changelog}{additional_information}\"")

        # push release
        run_command(f"git push origin {firmwareVersion}")

    else:
        print("Exiting without updating version")
        exit(1)


class FirmwareVersion():
    def __init__(self, version):
        self.major, self.minor, self.patch = version[1:].split(".")

    def __str__(self):
        return "v" + self.major + "." + self.minor + "." + self.patch


if __name__ == "__main__":
    release_firmware()
