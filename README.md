  Sistema de Processamento Paralelo de Requisições a um Banco de Dados

Este projeto simula um sistema gerenciador de requisições a um banco de dados utilizando múltiplos processos e threads, com comunicação via IPC (UNIX Domain Socket) e controle de concorrência com mutex.

O sistema é composto por dois processos principais:

   Processo Cliente

      • Envia requisições de inserção ou exclusão para o servidor.

      • A comunicação é feita por meio de pipe.

   Processo Servidor

      • Recebe as requisições do cliente.

      • Utiliza múltiplas threads para processá-las em paralelo.

      • As threads acessam uma estrutura simulada de banco de dados (um vetor) e utilizam mecanismos de sincronização para evitar conflitos de acesso.


   Operações Suportadas

     • INSERT: Adiciona um novo registro ao banco.

     • DELETE: Remove um registro com base no identificador.

     • SELECT: Busca e retorna dados com base em critérios simples (por exemplo, por ID).

     • UPDATE: Modifica um campo de um registro.


