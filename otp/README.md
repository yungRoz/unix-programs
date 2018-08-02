# One-Time Pad
A group of programs that use a One-Time Pad encryption scheme, multi-processing, and socket-based inter-process communication to encrypt and decrypt messages.

## Overview

Two of the programs, ```otp_enc_d``` & ```otp_dec_d``` function like daemons and are accessible using network sockets. When accessed, the daemon-like programs spawn a separate process to handle encryption or decryption transactions.
```otp_enc_d```: handles encryption requests
```otp_dec_d```: handles decryption requests
```keygen```: generates a key
```otp_enc```: sends key and plaintext to ```otp_enc_d``` to encrypt plaintext, thus, generating ciphertext  
```otp_dec```: sends key and ciphertext to ```otp_dec_d``` to decrypt ciphertext, thus, generating original plaintext

Both daemon-like programs support up to 5 concurrent connections.

### Run
1. Compile with the included compilation script ```compileall```
```
$./compileall
```
2. Run background programs:
```
$./otp_enc_d listening_port &
$./otp_dec_d listening_port &
```
3. Generate key:
```
$./keygen 2048 > mykey 
```

4. Encrypt text! Note: outputs ciphertext to stdout, so use any of the following:
```
$ otp_enc myplaintext mykey 57171
$ otp_enc myplaintext mykey 57171 > myciphertext
$ otp_enc myplaintext mykey 57171 > myciphertext &
```

5. Decrypt text! Also outputs to stdout
```
$ otp_enc myplaintext mykey 57171
$ otp_enc myplaintext mykey 57171 > myciphertext
$ otp_enc myplaintext mykey 57171 > myciphertext &
```

### Example Run
```
$ cat plaintext1
THE RED GOOSE FLIES AT MIDNIGHT
$ otp_enc_d 57171 &
$ otp_dec_d 57172 &
$ keygen 10 > myshortkey
$ otp_enc plaintext1 myshortkey 57171 > ciphertext1
Error: key ‘myshortkey’ is too short
$ echo $?
1
$ keygen 1024 > mykey
$ otp_enc plaintext1 mykey 57171 > ciphertext1
$ cat ciphertext1
GU WIRGEWOMGRIFOENBYIWUG T WOFL
$ keygen 1024 > mykey2
$ otp_dec ciphertext1 mykey 57172 > plaintext1_a
$ otp_dec ciphertext1 mykey2 57172 > plaintext1_b
$ cat plaintext1_a
THE RED GOOSE FLIES AT MIDNIGHT
$ cat plaintext1_b
WVIOWBTUEIOBC  FVTROIROUXA JBWE
$ cmp plaintext1 plaintext1_a
$ echo $?
0
$ cmp plaintext1 plaintext1_b
plaintext1 plaintext1_b differ: byte 1, line 1
$ echo $?
1
$ otp_enc plaintext5 mykey 57171
otp_enc error: input contains bad characters
$ otp_enc plaintext3 mykey 57172
Error: could not contact otp_enc_d on port 57172
$ echo $?
2
$
```
