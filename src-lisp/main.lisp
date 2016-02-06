;;;;
;;;; Copyright (c) 2016 Kamen Tomov, All Rights Reserved
;;;;
;;;; Redistribution and use in source and binary forms, with or without
;;;; modification, are permitted provided that the following conditions
;;;; are met:
;;;;
;;;;   * Redistributions of source code must retain the above copyright
;;;;     notice, this list of conditions and the following disclaimer.
;;;;
;;;;   * Redistributions in binary form must reproduce the above
;;;;     copyright notice, this list of conditions and the following
;;;;     disclaimer in the documentation and/or other materials
;;;;     provided with the distribution.
;;;;
;;;; THIS SOFTWARE IS PROVIDED BY THE AUTHOR 'AS IS' AND ANY EXPRESSED
;;;; OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
;;;; WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
;;;; ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
;;;; DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
;;;; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
;;;; GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
;;;; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
;;;; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
;;;; NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
;;;; SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;;;

(defpackage captcha
  (:use 
   #:common-lisp)
  (:export #:main))

(in-package :captcha)

(defvar +tolisp+ "/tmp/tolisp.tmp")
(defvar +fromlisp+ "/tmp/fromlisp.tmp")

(defun main (args)
  (declare (ignore args))
  (ignore-errors
    (delete-file +tolisp+))
  (ignore-errors
    (delete-file +fromlisp+))
  (sb-posix:mkfifo +tolisp+ (logxor sb-posix:S-IWUSR sb-posix:S-IRUSR))
  (sb-posix:mkfifo +fromlisp+ (logxor sb-posix:S-IWUSR sb-posix:S-IRUSR))
  (loop
     (with-open-file (in +tolisp+ :if-does-not-exist :error)
       (let ((line (or (read-line in) ""))) 
         (format t "Received ~d bytes from pipe tolisp, namely: ~s.~%"
                 (length line) line)
         (if (equal line "quit") (return))
         (with-open-file (out +fromlisp+ :direction :output
                              :if-exists :append :if-does-not-exist :error)
           (cond ((equal line "/question") (write-line (get-next) out))
                 ((equal (subseq line 0 (min 7 (length line))) "/answer")
                  (write-line (verify-answer (subseq line (min 8 (length line)))) out))
                 (t (write-line "-1" out))))))))

(let ((questions
       (vector '("How many planets are known to be inhabited by people?" . 1)
               '("How many hearts do people usually have?" . 1)
               '("If you have four apples and I eat two, how many are left?" . 2)))
      (index 0))
  (defun get-by-ix (ix) (aref questions (mod ix (length questions))))
  (defun get-next ()
    (prog1 (format nil "~a|~d" (car (get-by-ix index)) index)
      (incf index))))

(defun split-by-one-char (string &optional (chr #\Space))
  (loop for el in (loop for i = 0 then (1+ j)
		     as j = (position chr string :start i)
		     collect (subseq string i j)
		     while j)
     when (and (not (zerop (length el)))) collect el))

(defun request-query (query-string)
  (loop for part in (split-by-one-char query-string #\&)
     collect (let ((el (split-by-one-char part #\=)))
               (cons (first el) (second el)))))

(defun request-query-value (key req)
  (cdr (assoc key (request-query req) :test #'equal)))

(defun verify-answer (query-string)
  (block nil
    (let* ((err "-2")
           (ix (handler-case
                   (parse-integer (or (request-query-value "ix" query-string) ""))
                 (parse-error () (return err))))
           (guess (or (request-query-value "guess" query-string) (return err))))
      (format nil "~a|~a|~d" ix guess (cdr (get-by-ix ix))))))
