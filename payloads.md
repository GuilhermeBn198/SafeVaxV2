# SafeVax ESP32 — Payloads, Compressão e Criptografia

## 1. Estrutura do Payload JSON

Cada mensagem, antes de compressão e criptografia, tem o formato:

```json
{
  "temperatura_interna": 7.25,
  "umidade_interna": 45.80,
  "temperatura_externa": 26.10,
  "umidade_externa": 55.20,
  "estado_porta": "Fechada",
  "usuario": "ab12cd34",
  "alarm_state": "OK",
  "motivo": "periodic"
}
```

## 2. Compressão Huffman

1. **Árvore estática**: gerada a partir do `sampleAlphabet`.
2. **Mapeamento**: cada caractere → código binário (ex.: `{`→`1010`, `}`→`1101`, etc.).
3. **Compressão**: substitui cada caractere pelo seu código.

```cpp
String compressed = huffmanCompress(jsonString);
```

## 3. Criptografia AES-256-CBC

1. **Pad PKCS#7** para múltiplos de 16 bytes.  
2. **Encriptação** com chave de 256 bits e IV de 128 bits:

   ```cpp
   String cipher = aes_encrypt_b64(compressed);
   ```

3. **Envio**: `client.publish(topic, cipher.c_str());`

## 4. Descriptografia e Descompressão

No ESP32 (callback) ou em outro AP:

1. **Base64 → bytes**  
2. **AES-256-CBC decrypt** → padded compressed  
3. **Remove pad** → compressed  
4. **Huffman decompress** → JSON original  

   ```cpp
   String decrypted = aes_decrypt_b64(cipherText);
   String json = huffmanDecompress(decrypted, huffmanTree);
   ```

## 5. Exemplo Completo

```text
JSON → Huffman → AES → Base64  → MQTT
          ↑             ↑
      Descomprime  Descriptografa
```
