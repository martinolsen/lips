;;;;  read(r0: fd) => r0: ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Read, and construct, objects from fd.

read:
;;;;; Clear leading whitespace
    ; define whitespace characters
    mov %r2, 32                 ; Space
    mov %r3, 9                  ; Tab
    mov %r4, 10                 ; Newline

    ; loop label
    mov %r5, _clr_ws

    ; clear whitespace
_clr_ws:
    read %r1, %r0
    je %r5, %r1, %r2
    je %r5, %r1, %r3
    je %r5, %r1, %r4

;;;; Scan for read-table entry - TODO

;;;; Read token - TODO
