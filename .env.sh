#!/bin/bash


chcw_setup()
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
			 libfreetype-dev

	# i use arch & packman btw
	elif [ -f /etc/arch_release ]; then
		echo "running setup for arch"
		sudo pacman --noconfirm \
			 glew \
			 sdl2 \
			 glm \
			 freetype

	# for my loved nix weirdos
	elif command -v nix-shell &>/dev/null; then
		echo "running setup for nix"
		curl -o shell.nix \
			 https://raw.githubusercontent.com/callidaria/chclockwork/refs/heads/sources/shell.nix
		exec nix-shell

	# exotic people know whats next - setup yourself i don't know your ways
	else
		echo "ERROR: distro not natively supported. please install libs on your own."
	fi

	# texture headers
	mkdir -p core/include
	curl -o core/include/stb_image.h \
		 https://raw.githubusercontent.com/callidaria/chclockwork/refs/heads/sources/stb_image.h

	# script finished
	echo "done."
}


alias d='make debug'
alias da='make debug -B'
alias r='make release'
alias ra='make release -B'
alias e="./chcw"
