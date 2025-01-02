; This program will only work on x86 systems with BIOS support
[BITS 16]
[ORG 0x7C00]

start:
    ; Set up stack
    xor ax, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Read the first sector from the disk (boot sector)
    mov ah, 0x02        ; BIOS read sector function
    mov al, 0x01        ; Number of sectors to read
    mov ch, 0x00        ; Cylinder number
    mov cl, 0x02        ; Sector number (1-based, so 2 means first sector)
    mov dh, 0x00        ; Head number
    mov dl, 0x80        ; Drive number (0x80 for first hard disk)
    mov bx, 0x7E00      ; Buffer to store the sector
    int 0x13            ; BIOS interrupt

    jc read_error       ; Jump if carry flag is set (error)

    ; Check for FAT32
    mov si, 0x7E52      ; Offset for "FAT32" string in boot sector
    mov cx, 5           ; Length of "FAT32" string
    call check_fs
    jz fat32_detected

    ; Check for FAT16
    mov si, 0x7E36      ; Offset for "FAT16" string in boot sector
    mov cx, 5           ; Length of "FAT16" string
    call check_fs
    jz fat16_detected

    ; Check for NTFS
    mov si, 0x7E03      ; Offset for "NTFS" string in boot sector
    mov cx, 4           ; Length of "NTFS" string
    call check_fs
    jz ntfs_detected

    ; Unknown file system
    mov si, unknown_fs
    call print_string
    jmp done

fat32_detected:
    mov si, fat32_fs
    call print_string
    jmp done

fat16_detected:
    mov si, fat16_fs
    call print_string
    jmp done

ntfs_detected:
    mov si, ntfs_fs
    call print_string
    jmp done

read_error:
    mov si, read_error_msg
    call print_string

done:
    ; Hang the system
    cli
    hlt

check_fs:
    push cx
    push si
    mov di, 0x7E00      ; Buffer where sector is stored
    repe cmpsb
    pop si
    pop cx
    ret

print_string:
    mov ah, 0x0E        ; BIOS teletype function
.print_char:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .print_char
.done:
    ret

fat32_fs db 'FAT32 detected', 0
fat16_fs db 'FAT16 detected', 0
ntfs_fs db 'NTFS detected', 0
unknown_fs db 'Unknown file system', 0
read_error_msg db 'Error reading disk', 0

times 510-($-$$) db 0
dw 0xAA55