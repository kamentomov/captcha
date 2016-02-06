#
# Copyright (c) 2016 Kamen Tomov, All Rights Reserved
#

all: cgi-captcha lisp-captcha-server docs

cgi-captcha:
	$(MAKE) -C src-cgi

lisp-captcha-server:
	$(MAKE) -C src-lisp

docs:
	$(MAKE) -C docs

clean:
	@$(MAKE) -C docs $@
	@$(MAKE) -C src-cgi $@
	@$(MAKE) -C src-lisp $@

