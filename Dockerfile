FROM ubuntu:14.04

# Dependencies for model-t
RUN apt-get update --fix-missing \
    && apt-get install -y software-properties-common \
    && add-apt-repository -y ppa:team-gcc-arm-embedded/ppa \
    && apt-get update && apt-get install -y \
    freetype* \
    gcc-arm-embedded \
    libprotobuf-dev \
    netcat \
    protobuf-compiler \
    python-pip \
    python-protobuf \
    python-pystache \
    screen

# Dependencies for pygame (http://www.pygame.org/wiki/CompileUbuntu)
RUN apt-get install -y \
    fluid-soundfont-gm \
    fontconfig \
    fonts-freefont-ttf \
    git \
    libavcodec-dev \
    libavformat-dev \
    libfreetype6-dev \
    libportmidi-dev \
    libsdl1.2-dev \
    libsdl-image1.2-dev \
    libsdl-mixer1.2-dev \
    libsdl-ttf2.0-dev \
    libsmpeg-dev \
    libswscale-dev \
    libtiff5-dev \
    libx11-6 \
    libx11-dev \
    python-dev \
    python-numpy \
    python-opengl \
    xfonts-100dpi \
    xfonts-75dpi \
    xfonts-base \
    xfonts-cyrillic \
    && rm -rf /var/lib/apt/lists/*

RUN pip install Pygame==1.9.4

RUN useradd -d /brewbit -m -U -s /bin/bash brewbit \
    && mkdir /brewbit/model-t && chown brewbit:brewbit /brewbit/model-t

VOLUME /brewbit/model-t
WORKDIR /brewbit/model-t
COPY --chown=brewbit:brewbit . .
USER brewbit

RUN cd .. \
    && git clone https://github.com/brewbit/brewbit-protobuf-messages \
    && git clone https://github.com/ChibiOS/ChibiOS --branch stable_2.6.x \
    && git clone https://github.com/nanopb/nanopb \
    && cd nanopb && git checkout -b nanopb-0.2.4 nanopb-0.2.4

CMD make
