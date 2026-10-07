# huffman-compressor-c

Compressor de arquivo em C puro, sem dependência nenhuma, usando codificação de Huffman. Comprime e descomprime qualquer arquivo (texto ou binário) e devolve exatamente os mesmos bytes de volta.

```bash
./huffman comprimir arquivo.txt arquivo.huf
./huffman descomprimir arquivo.huf arquivo-restaurado.txt
```

## Como funciona, resumido

Conta quantas vezes cada um dos 256 valores de byte aparece no arquivo, monta uma árvore binária onde os bytes mais frequentes ficam mais perto da raiz (código mais curto) e os raros mais longe (código mais longo), e troca cada byte original pelo código da árvore. É o algoritmo clássico de Huffman, com fila de prioridade (heap mínimo) pra montar a árvore.

A árvore em si não é salva no arquivo comprimido — o cabeçalho guarda só a **tabela de frequências** (quantas vezes cada byte apareceu), e tanto compressão quanto descompressão reconstroem a mesma árvore a partir dela. Funciona porque a reconstrução é determinística: mesma tabela, mesma árvore, sempre (desempate por ordem de criação do nó, não por valor do byte).

## Compilando e testando

```bash
make          # gera o binário ./huffman
make test     # compila e roda os testes
```

7 testes com `assert`, sem framework: entrada vazia, arquivo de um único byte repetido, dois símbolos, texto com distribuição desigual (confere que realmente fica menor), os 256 valores de byte possíveis, cabeçalho inválido rejeitado, buffer curto demais rejeitado.

## O cabeçalho custa 1032 bytes fixos — e isso importa

Rodei de verdade com três tipos de arquivo pra ver o comportamento real, não assumido:

| Entrada | Tamanho original | Tamanho comprimido |
|---|---|---|
| Texto repetitivo (`"the quick brown fox..." x 200`) | 8.800 bytes | 5.882 bytes |
| Arquivo vazio | 0 bytes | 1.032 bytes |
| Binário aleatório (sem padrão) | 4.000 bytes | 5.023 bytes |

O texto repetitivo comprime bem (33% menor). Mas repara nos outros dois: arquivo vazio "comprime" pra 1.032 bytes, e o binário aleatório **cresce**. Isso não é bug, é a estrutura do formato: o cabeçalho sempre grava a tabela de frequência inteira — 256 posições de 4 bytes cada (1.024 bytes), mais 4 bytes de assinatura `"HUF1"` e 4 de tamanho original, dá 1.032 bytes fixos, não importa se o arquivo tem 5 bytes ou 5 milhões. Pra dado de alta entropia (já comprimido, aleatório, criptografado), não tem padrão de frequência pra explorar, então o resultado final é o cabeçalho fixo somado a um conteúdo que não encolheu — pode passar do tamanho original.

Uma implementação mais cuidadosa gravaria só as frequências dos símbolos que realmente aparecem (cabeçalho variável, menor pra arquivo pequeno ou com poucos símbolos distintos) em vez de sempre as 256. Não fiz essa otimização agora — o objetivo aqui era o algoritmo em si funcionando certo e testado, com o trade-off registrado, não espremer cada byte do formato.

## Limites que conheço

- Huffman por byte, não por padrão repetido maior (isso seria LZ77/LZ78, outro algoritmo). Não comprime repetição de sequência, só desbalanceamento de frequência de byte individual.
- Arquivo acima de ~4 GB não funciona: o tamanho é gravado em 4 bytes (`uint32_t`) no cabeçalho.
- Sem streaming — lê o arquivo inteiro pra memória antes de processar.
