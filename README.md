# SafeVax V2

O SafeVax é um sistema inovador voltado para o monitoramento e a segurança no armazenamento de vacinas em ambientes hospitalares. Desenvolvido para reduzir riscos e aumentar a eficiência no gerenciamento e administração de vacinas, o projeto utiliza sensores IoT (como DHT11 e RFID), compressão Huffman e criptografia AES para garantir que as condições ideais de conservação sejam sempre mantidas de forma segura e otimizada.

---

## Problema Abordado

O armazenamento adequado de vacinas é essencial para preservar sua eficácia e garantir a segurança dos pacientes. Em ambientes hospitalares, a violação das condições ideais — como a abertura frequente das portas dos refrigeradores e variações de temperatura — pode comprometer a qualidade dos insumos e, consequentemente, a saúde pública. Processos manuais de monitoramento estão sujeitos a falhas humanas, demandando uma solução automatizada, integrada e confiável.

---

## Solução Proposta

O SafeVax automatiza o monitoramento dos refrigeradores de vacinas através de:

- **Controle de Acesso:** Monitoramento da abertura e fechamento das portas do refrigerador, com registro detalhado dos acessos via sensor ultrassônico e RFID.  
- **Monitoramento de Temperatura:** Acompanhamento em tempo real da temperatura interna e externa (ambiente) com dois DHT11.  
- **Identificação via RFID:** Registro de quem acessa o refrigerador, possibilitando rastreabilidade e controle de acesso.  
- **Compressão de Dados (Huffman):** Redução do tamanho do payload JSON antes da transmissão, com árvore estática construída a partir do sampleAlphabet.  
- **Criptografia AES‑256‑CBC:** Proteção do payload comprimido usando chave de 256 bits e IV de 128 bits, garantindo confidencialidade em trânsito.  
- **Integração com Sistemas Hospitalares:** Sincronização de dados com plataformas de gestão de saúde para um monitoramento centralizado.  
- **Relatórios Automáticos & Alertas:** Geração de relatórios históricos e envio de alertas (via MQTT e WhatsApp) em caso de irregularidades.

### Melhorias e Funcionalidades Adicionais

- **Dashboard Interativo com Streamlit:** Consome as rotas HTTP do servidor Flask para visualização em tempo real e histórica.  
- **Segurança de Comunicação:**  
  - **HiveMQ TLS** para canal MQTT seguro.  
  - **AES‑256‑CBC** para criptografia de cada mensagem.  
- **Escalabilidade MQTT:** Cada ESP32 publica em `/centroDeVacinaXYZ/containerXYZ`, permitindo múltiplos containers.

---

## Credenciais e Parâmetros de Integração

| Item                          | Valor / Descrição                                                                                     |
|-------------------------------|-------------------------------------------------------------------------------------------------------|
| **Broker MQTT (HiveMQ)**      | `cd8839ea5ec5423da3aaa6691e5183a5.s1.eu.hivemq.cloud:8883` (TLS)                                       |
| **Username HiveMQ**           | `hivemq.webclient.1734636778463`                                                                      |
| **Password HiveMQ**           | `EU<pO3F7x?S%wLk4#5ib`                                                                                |
| **Tópico de Dados**           | `/<centroDeVacinaXYZ>/<containerXYZ>`                                                                |
| **Tópico de Controle**        | `esp32/control`                                                                                       |
| **Dicionário Huffman**        | Gerado a partir do _sampleAlphabet_ (veja `PAYLOADS.md` para mapa completo `char → código binário`) |
| **Chave AES‑256 (hex)**       | `A1B2C3D4E5F60718293A4B5C6D7E8F90112233445566778899AABBCCDDEEFF00`                                   |
| **IV AES‑128 (hex)**          | `00112233445566778899AABBCCDDEEFF`                                                                    |

> **Nota:** em produção, o IV deve ser gerado aleatoriamente por mensagem e prefixado ao payload criptografado.

---

## Referências

- **Código-fonte ESP32:** integra DHT11 interno/externo, ultrassônico, RFID, LCD, Huffman e AES.  
- **Servidor Flask + PostgreSQL:** coleta dados MQTT, armazena em tabelas separadas (`containers` e `dados_monitoramento`).  
- **Dashboard Streamlit:** consome rotas HTTP e exibe gráficos e tabelas.  
- **PAYLOADS.md:** documentação detalhada de payload JSON, compressão e criptografia.  
  → [Veja em PAYLOADS.md](./PAYLOADS.md)

---

## Desenvolvedores

- **Guilherme Lucas Pereira Bernardo** – _Líder de Projeto & Desenvolvedor_ ~ @GuilhermeBn198
- **Vinícius Brito** – _Especialista em Hardware_ ~ @ViniKhael  
- **Ruan Oliveira** – _Desenvolvedor de Software e Especialista em Saúde_ ~ @rouancb  
- **Gabriel Guilherme** – _Engenheiro de Dados_ ~ @Bounded-Bytes  

Para contribuir ou entrar em contato, consulte as informações no repositório.

---

SafeVax V2 é um passo importante rumo a um sistema de armazenamento de vacinas mais seguro, eficiente e confiável, promovendo a saúde pública e a excelência na gestão hospitalar.
