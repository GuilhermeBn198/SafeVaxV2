# huffman_decoder.py
import base64

# Dicionário Huffman invertido (binário -> caractere)
HUFFMAN_DICT_V1 = {
    "110": '"',
    "000": 'a',
    "0111": ' ',
    "0010": ',',
    "1111": 'e',
    "0100": 'r',
    "1010": 't',
    "10111": ':',
    "10000": '_',
    "01011": 'd',
    "10001": 'i',
    "10010": 'm',
    "01100": 'n',
    "00110": 'o',
    "01101": 'u',
    "001111": '0',
    "010100": '2',
    "010101": 'h',
    "111011": 'p',
    "100110": 's',
    "1001110": '1',
    "1001111": '3',
    "1011010": '7',
    "1110011": 'c',
    "1110101": 'x',
    "0011100": '{',
    "0011101": '}',
    "10110000": '4',
    "10110001": '5',
    "10110010": '6',
    "10110011": '9',
    "10110110": 'A',
    "10110111": 'E',
    "11100000": 'F',
    "11100001": 'L',
    "11100010": 'R',
    "11100011": 'T',
    "11100100": 'b',
    "11100101": 'g',
    "11101000": 'l',
    "11101001": 'v'
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
