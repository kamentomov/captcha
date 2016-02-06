#
# Copyright (c) 2016 Kamen Tomov, All Rights Reserved
#

TOP := $(dir $(lastword $(MAKEFILE_LIST)))
BUILDAPP=../../../$(TOP)/dists/quicklisp/software/buildapp-1.5.4/buildapp
QLPATH=../../../$(TOP)/

all: lisp-captcha-server

lisp-captcha-server:
	$(BUILDAPP) --output lisp-captcha-server --asdf-tree $(QLPATH) --load-system captcha --entry captcha:main

clean:
	@rm -f lisp-captcha-server 
	@rm -f *~ *.*~ \#*\# .#* *.fasl 
