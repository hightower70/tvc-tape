# TVCTape
Version: 0.2

The TVCTape converts between various tape file format of a vintage 8-bit computer, manufactured by Videoton in Hungary in late eighties. 
The program can handle frequency modulated audio signal in real time in both direction. This program can be used for direct data transfer between PC and TV Computer using audio connection on the PC and tape connector on the TVC. 

A following file formats are supported: *Wave In*, *Wave Out*, *WAV*, *CAS*, *BAS*, *TTP*, *ROM*, *BIN*, *HEX*

## Wave In/WAV file reading
The main goal of this software was to recover vintage programs stored on cassette tape. The tapes are severely degraded in the last few decades so some preprocessing is needed before the content can be decoded.
The first stage contains a digital pass-band filter.  There are two filter implemented, both has 1-3kHz band-pass range. One filter is a fast IIR filter with relatively wide roll-off range (this is the default for real time Wave In processing) the other filter is a slow FIR filter with narrow roll-off range, used for WAV file processing.  The filters can be changed by using _‘-p’_ command line switch.
A filtered signal is transferred to a intelligent signal limiter/gain control unit. This is intelligent because it receives information from the decoder about the appropriate signal presence. When no signal detected it only limits the amplitude of the signal, when an appropriate signal is detected (after a few periods on leading signal) it changes to gain control mode, when amplification is automatically applied when needed. The preprocessed signal can be saved to WAV file by using _‘-w’_ switch for further processing.
A filtered signal goes to the demodulator, where several methods used for processing the still low quality signal. It continuously adopts the internal timing to follow the speed changes of the tape, there is 128 times oversampling for the more accurate period length measurement, and the phase and position of the sync period is determined by a voting algorithm. 
The demodulated signal is further processed by the decoder. It generates the binary content from the incoming bit-stream. The CRC is calculated and checked, however in the case of mismatched CRCs the file is still saved (with an appended exclamation mark to the file name). So in the case of one or few bits error the content still can be recovered.
The TVCTape always uses the default Wave In device for signal source and the signal level must be adjusted until the yellow signal level marker just lit. It only accepts the 44.1kHz, 8 or 16 bits, PCM encoded WAV files.

Here is an exmaple of the wave in processing/cleaning. The frist wave form is the original audio data digitalized from the tape, and the lower waveform is digitally cleaned and restored waveform.
![tvctape_clean](https://user-images.githubusercontent.com/6670256/36795232-c06a16c0-1ca2-11e8-9120-19f3a9566f2a.png)

## Wave Out/WAV file saving
The ‘TVCTape’ is capable of generating the frequency modulated signal used for cassette data storage. For generating the signal it uses DDS (Direct Digital Synthesis) algorithm, so the important parameters of the generated signal can be changed. Using the _‘-g’_ switch the generated signal’s frequency can be shifted so a simple “turbo” loader can be implemented.
The program always uses the default Wave Out device. usually the volume must be at maximum in order to be processed by a real TVC. For WAV file it always uses 8 bit, 44.1kHz, PCM format.

## CAS file
This is the commonly used file format for storing programs on TVC. The TVCTape can only process CAS files which contains programs (data file processing might be added later). using the _‘-a’_ and _‘-c’_ switches the default values of the ‘autostart’ and ‘copyprotect’ flags can be altered respectively. 

## BAS file
The BAS file is a text file which contains BASIC program for the TVC. The file is processed according the TVC’s tokenizing rules. Because of the accented characters the file can be encoded in ANSII, UNICODE or UTF8. This can be controlled by the _‘-b’_ switch. If there is a binary data after the BASIC program (which is usually the case in the assembly programs) the binary part is saved as well using ‘BYTES’ directive and hex data format. 

## TTP file
This file format is generally used by the TVC emulators. This can be store more than one program file and can be loaded sequentially into the emulator as it comes from the tape. Using multiple files is easier by applying the _‘-l’_ or _‘-s’_ switches. The _‘-s’_ switch saves the list of the processed file names into the given text file, while _‘-l’_ loads the list of the files to be process from a text file specified by the switch.

## ROM file
This is the cartridge ROM format.This format is only supported for output file, input is not supported. The resulted file contains the binary image file which can be programmed into EPROM and connected to the cart port of the computer. Even BASIC programs can be converted to cart rom format. Using _'-r'_ switch the loader moudle can be selected from four possible codes. The loader 0 (_'-r 0'_ switch) doesn't uses compression or any border flashing effect. The loader 1 is without compression but flashing the border while loading the program from the cart to the main RAM. Loader '2' uses ZX7 compression to reduce the size of the cart rom image, so bigger than 16k programs can be converted as well. Dependig the compresibility of the given application 25~30k size application can be stored in the 16k cart. Loader '3' uses same compression but it flashes the border while decompressing and copying application from cart rom to the RAM.

## BIN file
This contains the pure binary content of any TVC program file without special header and file information. Using _‘-e’_ switch even the BASIC starter lines can be omitted.

## HEX file
Similar to the BIN file but the data is stored in Intel HEX file instead of the binary format. The _‘-e’_ switch can be used as well and there is an _‘-m’_ switch to change the memory start address stored in the hex file.
