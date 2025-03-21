# SafeVax V2

O SafeVax é um sistema inovador voltado para o monitoramento e a segurança no armazenamento de vacinas em ambientes hospitalares. Desenvolvido para reduzir riscos e aumentar a eficiência no gerenciamento e administração de vacinas, o projeto utiliza sensores IoT (como DHT11, RFID e sensores de abertura/fechamento de portas) para garantir que as condições ideais de conservação sejam sempre mantidas.

---

## Problema Abordado

O armazenamento adequado de vacinas é essencial para preservar sua eficácia e garantir a segurança dos pacientes. Em ambientes hospitalares, a violação das condições ideais — como a abertura frequente das portas dos refrigeradores e variações de temperatura — pode comprometer a qualidade dos insumos e, consequentemente, a saúde pública. Processos manuais de monitoramento estão sujeitos a falhas humanas, demandando uma solução automatizada, integrada e confiável.

---

## Solução Proposta

O SafeVax automatiza o monitoramento dos refrigeradores de vacinas através de:

- **Controle de Acesso:** Monitoramento da abertura e fechamento das portas do refrigerador, com registro detalhado dos acessos.
- **Monitoramento de Temperatura:** Acompanhamento em tempo real da temperatura interna, garantindo que os níveis ideais sejam mantidos.
- **Identificação via RFID:** Registro de quem acessa o refrigerador, possibilitando rastreabilidade e controle de acesso.
- **Integração com Sistemas Hospitalares:** Sincronização de dados com plataformas de gestão de saúde para um monitoramento centralizado.
- **Relatórios Automáticos:** Geração e análise de dados históricos, otimizando práticas de armazenamento e logística.
- **Alertas via WhatsApp:** Notificações automáticas para gestores em casos de irregularidades, permitindo uma resposta rápida.

### Melhorias e Funcionalidades Adicionais

- **Aplicativo Mobile Flutter:** Desenvolvimento de um app que recebe informações do servidor MQTT e envia alertas imediatos aos usuários, especialmente em casos críticos (ex.: elevação da temperatura).
- **Dashboard Interativo com Streamlit:** Disponibilização dos dados coletados de forma clara e legível, com gráficos, tabelas e relatórios para facilitar a análise.
- **Compactação de Dados:** Implementação de pelo menos dois algoritmos de compactação para otimizar a transmissão de dados entre os dispositivos e o servidor.
- **Criptografia de Comunicação:** Aplicação de um algoritmo robusto para criptografar os dados durante a comunicação, garantindo a segurança das informações transmitidas.

Esta abordagem integrada visa assegurar uma gestão mais eficiente e segura, minimizando os riscos e elevando a qualidade no armazenamento e administração das vacinas.

---

## Ferramentas Utilizadas

- **ESP32:** Microcontrolador robusto para integração e controle dos sensores.
- **Sensor DHT11:** Monitoramento em tempo real da temperatura interna.
- **Sensor de Abertura/Fechamento de Portas:** Registro preciso dos acessos.
- **Leitura de RFID:** Identificação dos usuários que acessam os equipamentos.
- **Relé Eletrônico:** Controle de acesso para abertura/fechamento das portas dos refrigeradores.
- **Servidor MQTT:** Responsável por receber e processar os dados enviados pelos dispositivos.
- **Integração com Sistemas Hospitalares:** Sincronização dos dados para monitoramento centralizado.
- **Alertas via WhatsApp:** Notificações automáticas para gestores em casos de irregularidades.
- **Aplicativo Mobile (Flutter):** Recebimento de dados e envio de alertas críticos.
- **Dashboard (Streamlit):** Visualização interativa e legível dos dados coletados.
- **Algoritmos de Compactação e Criptografia:** Otimização da transmissão e segurança dos dados.

---

## Desenvolvedores

O projeto SafeVax V2 é fruto da colaboração de uma equipe multidisciplinar comprometida com a inovação e a segurança na área da saúde:

- **Guilherme Lucas Pereira Bernardo** – *Líder de Projeto & Desenvolvedor*  
  Responsável pela coordenação geral do projeto, desenvolvimento do modelo de dados e integração das plataformas.

- **Vinícius Brito** – *Especialista em Hardware* ~ @ViniKhael  
  Focado na seleção, configuração e integração dos dispositivos eletrônicos utilizados.

- **Ruan Oliveira** – *Desenvolvedor de Software e Especialista em Saúde* ~ @rouancb 
  Responsável pela implementação dos módulos de monitoramento, notificação e futura integração via RFID.

- **Gabriel Guilherme** – *Engenheiro de Dados* ~ @Bounded-Bytes  
  Encarregado da análise e armazenamento dos dados, garantindo que os relatórios sejam precisos e informativos.

Se você deseja contribuir para o projeto ou entrar em contato com a equipe, por favor, consulte as informações de contato no repositório.

---

SafeVax V2 é um passo importante rumo a um sistema de armazenamento de vacinas mais seguro, eficiente e confiável, promovendo a saúde pública e a excelência na gestão hospitalar.
