import os
import shutil
import subprocess

def main():
    build_dir = "build"
    iso_boot_dir = os.path.join("iso", "boot", "grub")
    os.makedirs(build_dir, exist_ok=True)
    os.makedirs(iso_boot_dir, exist_ok=True)

    print("[*] Configuring i386 build with CMake...")
    subprocess.run(["cmake", "-S", ".", "-B", build_dir], check=True)

    print("[*] Compiling i386 Multiboot kernel...")
    subprocess.run(["cmake", "--build", build_dir], check=True)

    kernel_elf = os.path.join(build_dir, "kernel.elf")
    shutil.copy2(kernel_elf, os.path.join("iso", "boot", "kernel.elf"))

    iso_path = "vibecoded-os-i386-100mb.iso"
    print("[*] Packaging hybrid ISO image with macOS hdiutil...")
    subprocess.run([
        "hdiutil", "makehybrid", "-iso", "-joliet",
        "-default-volume-name", "VIBECODED100",
        "-o", iso_path, "iso"
    ], check=True)

    print(f"[+] Build complete! Kernel: {kernel_elf} | ISO: {iso_path}")
    print("[*] Run with: qemu-system-i386 -kernel build/kernel.elf -m 100M -vga std")

if __name__ == "__main__":
    main()
