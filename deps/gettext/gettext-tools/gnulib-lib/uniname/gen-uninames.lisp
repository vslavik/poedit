#!/usr/local/bin/clisp -C

;;; Creation of gnulib's uninames.h from the UnicodeData.txt table.
;;; Bruno Haible 2000-12-28

(defparameter add-comments nil)

(defstruct unicode-char
  (code nil :type integer)
  (name nil :type string)
  word-indices
  word-indices-index
)

(defstruct word-list
  (hashed nil :type hash-table)
  (sorted nil :type list)
  size                          ; number of characters total
  length                        ; number of words
)

(defun main (inputfile outputfile)
  (declare (type string inputfile outputfile))
  #+UNICODE (setq *default-file-encoding* charset:utf-8)
  (let ((all-chars '()))
    ;; Read all characters and names from the input file.
    (with-open-file (istream inputfile :direction :input)
      (loop
        (let ((line (read-line istream nil nil)))
          (unless line (return))
          (let* ((i1 (position #\; line))
                 (i2 (position #\; line :start (1+ i1)))
                 (code-string (subseq line 0 i1))
                 (code (parse-integer code-string :radix 16))
                 (name-string (subseq line (1+ i1) i2)))
            ; Ignore characters whose name starts with "<".
            (unless (eql (char name-string 0) #\<)
              ; Also ignore Hangul syllables; they are treated specially.
              (unless (<= #xAC00 code #xD7A3)
                ; Also ignore CJK compatibility ideographs; they are treated
                ; specially as well.
                (unless (or (<= #xF900 code #xFA2D) (<= #xFA30 code #xFA6A)
                            (<= #xFA70 code #xFAD9) (<= #x2F800 code #x2FA1D))
                  ; Transform the code so that it fits in 16 bits. In
                  ; Unicode 5.1 the following ranges are used.
                  ;   0x00000..0x04DFF  >>12=  0x00..0x04  -> 0x0..0x4
                  ;   0x0A000..0x0AAFF  >>12=  0x0A        -> 0x5
                  ;   0x0F900..0x0FFFF  >>12=  0x0F        -> 0x6
                  ;   0x10000..0x10A58  >>12=  0x10        -> 0x7
                  ;   0x12000..0x12473  >>12=  0x12        -> 0x8
                  ;   0x1D000..0x1D7FF  >>12=  0x1D        -> 0x9
                  ;   0x1F000..0x1F093  >>12=  0x1F        -> 0xA
                  ;   0x2F800..0x2FAFF  >>12=  0x2F        -> 0xB
                  ;   0xE0000..0xE00FF  >>12=  0xE0        -> 0xC
                  (flet ((transform (x)
                           (dpb
                             (case (ash x -12)
                               ((#x00 #x01 #x02 #x03 #x04) (ash x -12))
                               (#x0A 5)
                               (#x0F 6)
                               (#x10 7)
                               (#x12 8)
                               (#x1D 9)
                               (#x1F #xA)
                               (#x2F #xB)
                               (#xE0 #xC)
                               (t (error "Update the transform function for 0x~5,'0X" x))
                             )
                             (byte 8 12)
                             x
                        )) )
                    (push (make-unicode-char :code (transform code)
                                             :name name-string)
                          all-chars
            ) ) ) ) )
    ) ) ) )
    (setq all-chars (nreverse all-chars))
    ;; Split into words.
    (let ((words-by-length (make-array 0 :adjustable t)))
      (dolist (name (list* "HANGUL SYLLABLE" "CJK COMPATIBILITY" (mapcar #'unicode-char-name all-chars)))
        (let ((i1 0))
          (loop
            (when (>= i1 (length name)) (return))
            (let ((i2 (or (position #\Space name :start i1) (length name))))
              (let* ((word (subseq name i1 i2))
                     (len (length word)))
                (when (>= len (length words-by-length))
                  (adjust-array words-by-length (1+ len))
                )
                (unless (aref words-by-length len)
                  (setf (aref words-by-length len)
                        (make-word-list
                          :hashed (make-hash-table :test #'equal)
                          :sorted '()
                ) )     )
                (let ((word-list (aref words-by-length len)))
                  (unless (gethash word (word-list-hashed word-list))
                    (setf (gethash word (word-list-hashed word-list)) t)
                    (push word (word-list-sorted word-list))
                ) )
              )
              (setq i1 (1+ i2))
      ) ) ) )
      ;; Sort the word lists.
      (dotimes (len (length words-by-length))
        (unless (aref words-by-length len)
          (setf (aref words-by-length len)
                (make-word-list
                  :hashed (make-hash-table :test #'equal)
                  :sorted '()
        ) )     )
        (let ((word-list (aref words-by-length len)))
          (setf (word-list-sorted word-list)
                (sort (word-list-sorted word-list) #'string<)
          )
          (setf (word-list-size word-list)
                (reduce #'+ (mapcar #'length (word-list-sorted word-list)))
          )
          (setf (word-list-length word-list)
                (length (word-list-sorted word-list))
      ) ) )
      ;; Output the tables.
      (with-open-file (ostream outputfile :direction :output
                       #+UNICODE :external-format #+UNICODE charset:ascii)
        (format ostream "/* DO NOT EDIT! GENERATED AUTOMATICALLY! */~%")
        (format ostream "/*~%")
        (format ostream " * ~A~%" (file-namestring outputfile))
        (format ostream " *~%")
        (format ostream " * Unicode character name table.~%")
        (format ostream " * Generated automatically by the gen-uninames utility.~%")
        (format ostream " */~%")
        (format ostream "~%")
        (format ostream "static const char unicode_name_words[~D] = {~%"
                        (let ((sum 0))
                          (dotimes (len (length words-by-length))
                            (let ((word-list (aref words-by-length len)))
                              (incf sum (word-list-size word-list))
                          ) )
                          sum
        )               )
        (dotimes (len (length words-by-length))
          (let ((word-list (aref words-by-length len)))
            (dolist (word (word-list-sorted word-list))
              (format ostream " ~{ '~C',~}~%" (coerce word 'list))
        ) ) )
        (format ostream "};~%")
        (format ostream "#define UNICODE_CHARNAME_NUM_WORDS ~D~%"
                        (let ((sum 0))
                          (dotimes (len (length words-by-length))
                            (let ((word-list (aref words-by-length len)))
                              (incf sum (word-list-length word-list))
                          ) )
                          sum
        )               )
        #| ; Redundant data
        (format ostream "static const uint16_t unicode_name_word_offsets[~D] = {~%"
                        (let ((sum 0))
                          (dotimes (len (length words-by-length))
                            (let ((word-list (aref words-by-length len)))
                              (incf sum (word-list-length word-list))
                          ) )
                          sum
        )               )
        (dotimes (len (length words-by-length))
          (let ((word-list (aref words-by-length len)))
            (when (word-list-sorted word-list)
              (format ostream " ")
              (do ((l (word-list-sorted word-list) (cdr l))
                   (offset 0 (+ offset (length (car l)))))
                  ((endp l))
                (format ostream "~<~% ~0,79:; ~D,~>" offset)
              )
              (format ostream "~%")
        ) ) )
        (format ostream "};~%")
        |#
        (format ostream "static const struct { uint16_t extra_offset; uint16_t ind_offset; } unicode_name_by_length[~D] = {~%"
                        (1+ (length words-by-length))
        )
        (let ((extra-offset 0)
              (ind-offset 0))
          (dotimes (len (length words-by-length))
            (let ((word-list (aref words-by-length len)))
              (format ostream "  { ~D, ~D },~%" extra-offset ind-offset)
              (incf extra-offset (word-list-size word-list))
              (incf ind-offset (word-list-length word-list))
          ) )
          (format ostream "  { ~D, ~D }~%" extra-offset ind-offset)
        )
        (format ostream "};~%")
        (let ((ind-offset 0))
          (dotimes (len (length words-by-length))
            (let ((word-list (aref words-by-length len)))
              (dolist (word (word-list-sorted word-list))
                (setf (gethash word (word-list-hashed word-list)) ind-offset)
                (incf ind-offset)
        ) ) ) )
        (dolist (word '("HANGUL" "SYLLABLE" "CJK" "COMPATIBILITY"))
          (format ostream "#define UNICODE_CHARNAME_WORD_~A ~D~%" word
                          (gethash word (word-list-hashed (aref words-by-length (length word))))
        ) )
        ;; Compute the word-indices for every unicode-char.
        (dolist (uc all-chars)
          (let ((name (unicode-char-name uc))
                (indices '()))
            (let ((i1 0))
              (loop
                (when (>= i1 (length name)) (return))
                (let ((i2 (or (position #\Space name :start i1) (length name))))
                  (let* ((word (subseq name i1 i2))
                         (len (length word)))
                    (push (gethash word (word-list-hashed (aref words-by-length len)))
                          indices
                    )
                  )
                  (setq i1 (1+ i2))
            ) ) )
            (setf (unicode-char-word-indices uc)
                  (coerce (nreverse indices) 'vector)
            )
        ) )
        ;; Sort the list of unicode-chars by word-indices.
        (setq all-chars
              (sort all-chars
                    (lambda (vec1 vec2)
                      (let ((len1 (length vec1))
                            (len2 (length vec2)))
                        (do ((i 0 (1+ i)))
                            (nil)
                          (if (< i len2)
                            (if (< i len1)
                              (cond ((< (aref vec1 i) (aref vec2 i)) (return t))
                                    ((> (aref vec1 i) (aref vec2 i)) (return nil))
                              )
                              (return t)
                            )
                            (return nil)
                    ) ) ) )
                    :key #'unicode-char-word-indices
        )     )
        ;; Output the word-indices.
        (format ostream "static const uint16_t unicode_names[~D] = {~%"
                        (reduce #'+ (mapcar (lambda (uc) (length (unicode-char-word-indices uc))) all-chars))
        )
        (let ((i 0))
          (dolist (uc all-chars)
            (format ostream " ~{ ~D,~}"
                            (maplist (lambda (r) (+ (* 2 (car r)) (if (cdr r) 1 0)))
                                     (coerce (unicode-char-word-indices uc) 'list)
                            )
            )
            (when add-comments
              (format ostream "~40T/* ~A */" (unicode-char-name uc))
            )
            (format ostream "~%")
            (setf (unicode-char-word-indices-index uc) i)
            (incf i (length (unicode-char-word-indices uc)))
        ) )
        (format ostream "};~%")
        (format ostream "static const struct { uint16_t code; uint32_t name:24; }~%")
        (format ostream "#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)~%__attribute__((__packed__))~%#endif~%")
        (format ostream "unicode_name_to_code[~D] = {~%"
                        (length all-chars)
        )
        (dolist (uc all-chars)
          (format ostream "  { 0x~4,'0X, ~D },"
                          (unicode-char-code uc)
                          (unicode-char-word-indices-index uc)
          )
          (when add-comments
            (format ostream "~21T/* ~A */" (unicode-char-name uc))
          )
          (format ostream "~%")
        )
        (format ostream "};~%")
        (format ostream "static const struct { uint16_t code; uint32_t name:24; }~%")
        (format ostream "#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)~%__attribute__((__packed__))~%#endif~%")
        (format ostream "unicode_code_to_name[~D] = {~%"
                        (length all-chars)
        )
        (dolist (uc (sort (copy-list all-chars) #'< :key #'unicode-char-code))
          (format ostream "  { 0x~4,'0X, ~D },"
                          (unicode-char-code uc)
                          (unicode-char-word-indices-index uc)
          )
          (when add-comments
            (format ostream "~21T/* ~A */" (unicode-char-name uc))
          )
          (format ostream "~%")
        )
        (format ostream "};~%")
        (format ostream "#define UNICODE_CHARNAME_MAX_LENGTH ~D~%"
                        (reduce #'max (mapcar (lambda (uc) (length (unicode-char-name uc))) all-chars))
        )
        (format ostream "#define UNICODE_CHARNAME_MAX_WORDS ~D~%"
                        (reduce #'max (mapcar (lambda (uc) (length (unicode-char-word-indices uc))) all-chars))
        )
      )
) ) )

(main (first *args*) (second *args*))
