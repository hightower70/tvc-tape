U0RETADDR		EQU 880h
U0SPACE			EQU 882h
BASICPRGSTART	EQU 19EFh
BASFLG			EQU 0EB6h

DECOMPRESSOR_ENABLED EQU 0
BORDERBLINKING_ENABLED EQU 1

		org 0C000h
		db "MOPS"

		; Set memory map to: U0, U1, U2, CART
		ld	a, 30h
		ld	(3), a
		out	(2), a
		
		; calculate returning address for 2.x ROM
		ld	hl,-22
		add hl,de
		ld (U0RETADDR),hl
		
		; Copy U0 program to the RAM
		ld	hl, U0PRG
		ld	de, U0SPACE
		ld	bc, U0PRGLEN
		ldir
		
		; call BASIC area initialize
		call	BASINIT

	if DECOMPRESSOR_ENABLED != 0
	    ; Decompress CART content to RAM
		ld	hl,CARTPRGSTART
		ld	de,BASICPRGSTART
  
		call dzx7_standard
	else
		; copy CART content to RAM
		ld	hl, CARTPRGSTART
		ld	de, BASICPRGSTART
		ld	bc, (CARTPROGSIZE)
	if BORDERBLINKING_ENABLED
cartcopy:
		ld  a,(hl)
		ldi
		out	(0),a
		ld  a, b
		or  c
		jp  nz,cartcopy
	else
		ldir
	endif
	endif

	if BORDERBLINKING_ENABLED != 0
		xor a,a
		out (0), a
	endif

	; setup BASIC program location
		ld	hl, BASICPRGSTART
		ld	(1720h), hl
		ld	(1722h), hl

		; Start program
		ld	a,(BASVER)
		cp	2
		jp	nz, BASRUN
		
		; set BASIC flag to no start screen, no prompt, no new command and autostart
		ld	a, 0fh
		ld	(BASFLG), a

		; load return address
		ld	hl,(U0RETADDR)
				
		; Set memory map to: SYS, U1, U2, CART
		ld	a, 20h
		out	(2), a

		; return to ROM
		jp	(hl)
			
		; U0 program for calling functions from SYS ROM
U0PRG:
		.phase U0SPACE
BASINIT:	
		; Set memory map to: U0, U1, U2, SYS
		ld	a, 70h
		ld	(3), a
		out	(2), a
		
		; determine ROM version (1.x or 2.x)
		ld 	a, (0da0ch)
		cp	5bh
		jr 	nz,INITEND
		
		; version is 1.x -> store version
		ld	a,1
		ld	(BASVER),a
		
		; initialize BASIC storage area
		ld	hl,1700h
		push 	hl
		pop	ix
		ld	bc,02efh
		ld	de,1701h
		ld	(hl),0
		ldir
		
		; initialize error handlers
		ld	hl,0fb5bh
		ld 	de,8
		ld	bc,27h
		ldir
		
		; call NEW command
		call	0de10h

INITEND:	
		; Set memory map to: U0, U1, U2, CART
		ld	a, 30h
		ld	(3), a
		out	(2), a
		
		; return back to the CART
		ret
		
		; ROM version information
BASVER	db 2

		; Set memory map to: U0, U1, U2, SYS
BASRUN:	ld	a, 70h
		ld	(3), a
		out	(2), a
		
		; enable interrupts
		ei
		
		; execute RUN command
		ld	hl, (1722h)
		jp	0de23h
		
		.dephase
U0PRGLEN:	equ $-U0PRG

	if DECOMPRESSOR_ENABLED != 0
; -----------------------------------------------------------------------------
; ZX7 decoder by Einar Saukas, Antonio Villena & Metalbrain
; "Standard" version (69 bytes only)
; -----------------------------------------------------------------------------
; Parameters:
;   HL: source address (compressed data)
;   DE: destination address (decompressing)
; -----------------------------------------------------------------------------

dzx7_standard:
        ld      a, 80h
dzx7s_copy_byte_loop:
        ldi                             ; copy literal byte
dzx7s_main_loop:
        call    dzx7s_next_bit
        jr      nc, dzx7s_copy_byte_loop ; next bit indicates either literal or sequence

; determine number of bits used for length (Elias gamma coding)
        push    de
        ld      bc, 0
        ld      d, b
dzx7s_len_size_loop:
        inc     d
        call    dzx7s_next_bit
        jr      nc, dzx7s_len_size_loop

; determine length
dzx7s_len_value_loop:
        call    nc, dzx7s_next_bit
        rl      c
        rl      b
        jr      c, dzx7s_exit           ; check end marker
        dec     d
        jr      nz, dzx7s_len_value_loop
        inc     bc                      ; adjust length

; determine offset
        ld      e, (hl)                 ; load offset flag (1 bit) + offset value (7 bits)
        inc     hl
		scf
		rl		e
        jr      nc, dzx7s_offset_end    ; if offset flag is set, load 4 extra bits
        ld      d, $10                  ; bit marker to load 4 bits
dzx7s_rld_next_bit:
        call    dzx7s_next_bit
        rl      d                       ; insert next bit into D
        jr      nc, dzx7s_rld_next_bit  ; repeat 4 times, until bit marker is out
        inc     d                       ; add 128 to DE
        srl	d							; retrieve fourth bit from D
dzx7s_offset_end:
        rr      e                       ; insert fourth bit into E

; copy previous sequence
        ex      (sp), hl                ; store source, restore destination
        push    hl                      ; store destination
        sbc     hl, de                  ; HL = destination - offset - 1
        pop     de                      ; DE = destination
        ldir
dzx7s_exit:
        pop     hl                      ; restore source address (compressed data)
        jr      nc, dzx7s_main_loop
dzx7s_next_bit:
        add     a, a                    ; check next bit
        ret     nz                      ; no more bits left?
		
	
        ld      a, (hl)                 ; load another group of 8 bits
	if BORDERBLINKING_ENABLED != 0
		out (0), a
	endif

        inc     hl
        rla
        ret
	endif
		
CARTPROGSIZE:
		; two bytes are reserved for program size

		; CART data (compressed) starts here
CARTPRGSTART equ CARTPROGSIZE + 2

		END	