#!/bin/bash


GPU_VULKAN_MODE=true
GPU_BUILD_SUFFIX="-DVKBUILD"
GPU_CURRENT_MODE="vulkan mode (default)"

cw_setup()
{
	echo "running linux project setup"

	# distro detection
	# debian & aptitude
	if [ -f /etc/debian_version ]; then
		echo "running setup for debian"
		sudo apt install -y \
			 libglew-dev \
			 libsdl2-dev \
			 libglm-dev \
			 libassimp-dev \
			 libfreetype-dev \
			 valgrind \
			 kcachegrind

	# i use arch & packman btw
	elif [ -f /etc/arch_release ]; then
		echo "running setup for arch"
		sudo pacman --noconfirm \
			 glew \
			 sdl2 \
			 glm \
			 assimp \
			 freetype \
			 valgrind \
			 kcachegrind

	# for my loved nix weirdos
	elif command -v nix-shell &>/dev/null; then
		echo "running setup for nix"
		curl -o shell.nix \
			 https://raw.githubusercontent.com/callidaria/chclockwork/refs/heads/sources/shell.nix
		exec nix-shell

	# windows library warnings
	elif [ "$OS" == "Windows_NT" ]; then
		echo "WARNING: you are running the auto-setup on windows. Trying to auto-setup with MSYS2!"
		pacman -S --noconfirm \
			   mingw-w64-x86_64-gcc \
			   mingw-w64-x86_64-make \
			   mingw-w64-x86_64-glew \
			   mingw-w64-x86_64-SDL2 \
			   mingw-w64-x86_64-glm \
			   mingw-w64-x86_64-assimp \
			   mingw-w64-x86_64-freetype

	# exotic people know whats next - setup yourself i don't know your ways
	else
		echo "ERROR: distro not natively supported!"
		echo "please install libs on your own & submit what's missing."
	fi

	# texture headers
	mkdir -p core/include
	curl -o core/include/stb_image.h \
		 https://raw.githubusercontent.com/callidaria/chclockwork/refs/heads/sources/stb_image.h

	# script finished
	echo "done."
}

sc()
{
	# filesystem stuffs
	SHADER_DIR="./core/shader/vulkan/"
	BINARY_DIR="$SHADER_DIR"bin/
	mkdir -p "$BINARY_DIR"

	# iterate shader files
	for shader in "$SHADER_DIR"*; do
		if [ -f "$shader" ]; then
			time_start=$(date +%s%3N)
			file=$(basename "$shader")
			printf "compiling shader %-75s%s" "$file"

			# check if shader binary is outdated to skip unnecessary recompiles
			compile=1
			other_file="${BINARY_DIR}${file}"
			if [ -f "$other_file" ]; then
				stamp0=$(stat -c %Y "$shader")
				stamp1=$(stat -c %Y "$other_file")
				compile=$((stamp0>stamp1))
			fi

			# compile glsl shader code to spir-v
			if ((compile)); then
				glslc "$shader" -o "$other_file"
			fi

			# compile time output
			time_end=$(date +%s%3N)
			time_delta=$((time_end-time_start))
			printf "| done in %sms\n" "$time_delta"
		fi
	done
}

cgl()
{
	echo "currently building ${GPU_CURRENT_MODE}"
}

sgl()
{
	if $GPU_VULKAN_MODE; then
		GPU_VULKAN_MODE=false
		GPU_BUILD_SUFFIX="-DGLBUILD"
		GPU_CURRENT_MODE="opengl mode"
	else
		GPU_VULKAN_MODE=true
		GPU_BUILD_SUFFIX="-DVKBUILD"
		GPU_CURRENT_MODE="vulkan mode"
	fi
	echo "${GPU_CURRENT_MODE} enabled"
}


cw_memfix()
{
	valgrind --leak-check=full ./chcw
}


cw_profile()
{
	sudo valgrind --tool=callgrind --dump-instr=yes --simulate-cache=yes --collect-jumps=yes ./chcw
	cgfile=$(ls -t callgrind.out.* | head -n 1)
	if [ -f "$cgfile" ]; then
		sudo chmod 777 "$cgfile"
		kcachegrind "$cgfile"
		rm "$cgfile"
	else
		echo "error creating profiler output"
	fi
}


cw_help()
{
	printf "C. Hansen's Counter-Clockwork Environment Helpdesk:\n\n"
	printf "%-15s - %s\n" "cw_help" "i didn't need to tell you that for recursive reasons"
	printf "%-15s - %s\n" "cw_setup" "project setup for build & development purposes"
	printf "%-15s - %s\n" "cw_memfix" "run the engine with memory checking enabled for console output"
	printf "%-15s - %s\n" "cw_profile" "run the engine with cpu performance profiling & open for analysis after"
	printf "%-15s - %s\n" "cgl" "show currently active graphics library for next version build"
	printf "%-15s - %s\n" "sgl" "switch graphics api library to specify next version build"
	printf "%-15s - %s\n" "d" "build debug (only outdated libs)"
	printf "%-15s - %s\n" "da" "build debug, force build all libs"
	printf "%-15s - %s\n" "r" "build release (only outdated libs). WARNING: will not override debug versions!"
	printf "%-15s - %s\n" "ra" "build release, force build all libs"
	printf "%-15s - %s\n" "sc" "compile shader binaries for vulkan version"
	printf "%-15s - %s\n" "e" "execute engine binary"
}


alias d='make debug GPUAPI_SUFFIX="${GPU_BUILD_SUFFIX}"'
alias da='make debug -B GPUAPI_SUFFIX="${GPU_BUILD_SUFFIX}"'
alias r='make release GPUAPI_SUFFIX="${GPU_BUILD_SUFFIX}"'
alias ra='make release -B GPUAPI_SUFFIX="${GPU_BUILD_SUFFIX}"'

if [ "$OS" == "Windows_NT" ]; then
	alias e="./chcw.exe"
else
	alias e="./chcw"
	alias ea="valgrind --suppressions=gfxapi.supp ./chcw"
fi
