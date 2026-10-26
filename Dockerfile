FROM ubuntu:24.04
RUN DEBIAN_FRONTEND=noninteractive apt -y update && \
    DEBIAN_FRONTEND=noninteractive apt -y upgrade && \
    DEBIAN_FRONTEND=noninteractive apt -y install \
# gem5 dependencies
    build-essential scons python3-dev git pre-commit zlib1g zlib1g-dev \
    libprotobuf-dev protobuf-compiler libprotoc-dev libgoogle-perftools-dev \
    libboost-all-dev libhdf5-serial-dev python3-pydot python3-venv python3-tk \
    mypy m4 libcapstone-dev libpng-dev libelf-dev pkg-config wget cmake doxygen \
# HOME-V dependencies
    libxxhash-dev mold python3-pip ninja-build
# Install cross-compiler with uclibc to generate binaries with `-nostdlib` that work with gem5
RUN cd /opt && \
    wget https://toolchains.bootlin.com/downloads/releases/toolchains/aarch64/tarballs/aarch64--uclibc--stable-2024.05-1.tar.xz && \
    tar xf aarch64--uclibc--stable-2024.05-1.tar.xz && mv aarch64--uclibc--stable-2024.05-1 aarch64 && \
    wget https://toolchains.bootlin.com/downloads/releases/toolchains/riscv64-lp64d/tarballs/riscv64-lp64d--uclibc--stable-2025.08-1.tar.xz && \
    tar xf riscv64-lp64d--uclibc--stable-2025.08-1.tar.xz && mv riscv64-lp64d--uclibc--stable-2025.08-1 riscv64 && \
    for b in addr2line ar as c++filt cpp elfedit g++ gcc gcc-ar gcc-nm gcc-ranlib gcov gcov-dump gcov-tool gprof ld ld.bfd nm objcopy objdump ranlib readelf size strings strip; do \
    update-alternatives --install /usr/bin/aarch64-linux-${b} aarch64-linux-${b} /opt/aarch64/bin/aarch64-linux-${b} 30 && \
    update-alternatives --install /usr/bin/riscv64-linux-${b} riscv64-linux-${b} /opt/riscv64/bin/riscv64-linux-${b} 30; done
