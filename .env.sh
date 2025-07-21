#!/bin/bash


chcw_setup()
{
	echo "running linux project setup"
	mkdir -p core/include
	curl -o core/include/stb_image.h \
		 https://raw.githubusercontent.com/callidaria/chclockwork/refs/heads/sources/stb_image.h
	echo "TODO additional libraries still pose a problem"
	echo "done."
}


alias d='make debug'
alias da='make debug -B'
alias r='make release'
alias ra='make release -B'
