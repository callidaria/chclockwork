#!/bin/bash


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

	# exotic people know whats next - setup yourself i don't know your ways
	else
		echo "ERROR: distro not natively supported. please install libs on your own & submit what's missing."
	fi

	# texture headers
	mkdir -p core/include
	curl -o core/include/stb_image.h \
		 https://raw.githubusercontent.com/callidaria/chclockwork/refs/heads/sources/stb_image.h

	# script finished
	echo "done."
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
	printf "C. Hanson's Clockwork Environment Helpdesk:\n\n"
	printf "%-15s - %s\n" "cw_help" "i didn't need to tell you that for recursive reasons"
	printf "%-15s - %s\n" "cw_setup" "project setup for build & development purposes"
	printf "%-15s - %s\n" "cw_memfix" "run the engine with memory checking enabled for console output"
	printf "%-15s - %s\n" "cw_profile" "run the engine with cpu performance profiling & open for analysis after"
	printf "%-15s - %s\n" "d" "build debug (only outdated libs)"
	printf "%-15s - %s\n" "da" "build debug, force build all libs"
	printf "%-15s - %s\n" "r" "build release (only outdated libs). WARNING: will not override debug versions!"
	printf "%-15s - %s\n" "ra" "build release, force build all libs"
	printf "%-15s - %s\n" "e" "execute engine binary"
}


alias d='make debug'
alias da='make debug -B'
alias r='make release'
alias ra='make release -B'
alias e="./chcw"
