# huffman_decoder.py
import base64

# Dicionário Huffman invertido (binário -> caractere)
HUFFMAN_DICT_V1 = {
    "000": '"',   "0010": ':',   "0011": ',',   "0100": 'a',
    "0101": 'e',  "0110": 'i',   "0111": 'o',   "1000": 'r',
    "1001": 't',  "1010": 'm',   "1011": 'u',   "1100": 'd',
    "11010": 'p', "11011": 's',  "11100": '_',  "11101": 'C',
    "11110": 'F', "11111": 'A',  "00000": 'L',  "00001": 'R',
    "00010": 'D', "00011": '0',  "00100": '1',  "00101": '2',
    "00110": '3', "00111": '4',  "01000": '5',  "01001": '6',
    "01010": '7', "01011": '8',  "01100": '9',  "01101": '.',
    "01110": 'E', "01111": 'c',  "10000": 'n',  "10001": 'h',
    "10010": 'v', "10011": 'g',  "10100": 'l',  "10101": 'b',
    "10110": 'k', "10111": 'O',  "11000": 'K',  "11001": 'I',
    "110001": 'T',"110010": 'U', "110011": 'M', "110100": ' '
}

def base64_to_binary(b64_str):
    raw = base64.b64decode(b64_str)
    return ''.join(f"{byte:08b}" for byte in raw)

def huffman_decompress(encoded_b64, version="v1"):
    if version != "v1":
        raise ValueError(f"Versão de dicionário Huffman não suportada: {version}")

    binary = base64_to_binary(encoded_b64)
    decoded = ""
    i = 0

    # Ordena as chaves do dicionário da maior para a menor
    sorted_keys = sorted(HUFFMAN_DICT_V1.keys(), key=len, reverse=True)

    while i < len(binary):
        match = False
        for key in sorted_keys:
            if binary.startswith(key, i):
                decoded += HUFFMAN_DICT_V1[key]
                i += len(key)
                match = True
                break
        if not match:
            print(f"[Erro] Nenhum match para bits a partir de i={i}: {binary[i:i+10]}")
            break  # Pode levantar exceção se quiser
    return decoded
