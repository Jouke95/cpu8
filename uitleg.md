# cpu8 — Uitleg bij de evaluatievragen

## 1. Wat gebeurt er stap voor stap als je CPU een `JZ` tegenkomt?

1. **Fetch.** `fetch()` leest `mem[pc]` (byte 1) en verhoogt de `pc` met 1.
2. **Decode.** De bovenste drie bits (`byte >> 5`) zijn `101`: dat is `JZ`.
3. **Execute.** `jz()` haalt byte 2 op met `fetch()`, het doeladres. De `pc` is nu 2 verder dan het begin van de instructie en wijst naar de *volgende* instructie.
4. **Voorwaarde.** Staat de flag op 1 (het laatste `ADD`/`SUB`-resultaat was 0), dan wordt `pc = doeladres`. Staat de flag op 0, dan gebeurt er niets met de `pc`: die wijst al naar de volgende instructie, dus het programma loopt gewoon door.

Voorbeeld uit het aftel-programma: `JZ end` staat op adres 8 en 9. Na de twee fetches is `pc = 10`.
Is de flag 0, dan gaat de CPU verder met de `JMP` op 10. Is de flag 1, dan wordt `pc = 12` en gaat hij verder bij `end:`.

Byte 2 haal je altijd op, ook als je niet springt. Je weet pas na het ophalen wat het doeladres is, en alleen zo
staat de `pc` aan het eind op de volgende instructie. Zou je byte 2 bij een niet-genomen sprong overslaan, dan
lees je de adreswaarde als opcode.

## 2. Waarom dit instructieformaat (3 bits opcode, 2 registerbits, 1 mode-bit, aparte byte 2)?

- **3 bits opcode:** er zijn 8 instructies, en 2³ = 8. Dat is genoeg voor een simpele CPU waar je al veel mee kunt.
- **2 registerbits (bit 4 en 3):** `ADD` en `SUB` hebben een bestemming en een bron nodig. Met één bit per register kies je A of B, en dat geeft ook `ADD A A` en `SUB B A`.
- **1 mode-bit (bit 2):** `LOAD` heeft twee betekenissen, een getal laden of de inhoud van een adres laden. Dat lost je op zonder een negende opcode, want alle acht opcodes zijn al in gebruik.
- **Aparte byte 2:** een adres is 8 bits (256 bytes geheugen). Dat past alleen als byte 2 een volledig byte is. Een klein operandveld in byte 1 zou nooit alle adressen bereiken. De prijs: `ADD`, `SUB`, `PRINT` en `HALT` verspillen byte 2 als padding. Dat vergoedt de vaste lengte: elke instructie is 2 bytes, dus de `pc` gaat steeds +2 en de decoder blijft simpel.
- **2 reserve-bits (bit 1 en 0):** ruimte om later uit te breiden.

**Alternatief dat ik heb laten liggen:** 4 bits opcode. Dat geeft 16 instructies, maar kost een reserve-bit en
verschuift dst, src en mode een plek op. Voor dit project heb ik 8 instructies genoeg gevonden en wilde ik de
uitbreidingsruimte houden.

## 3. Het verschil tussen een register en het geheugen

Een **register** is waar de CPU op dit moment *mee werkt*: `ADD` en `SUB` kunnen alleen rekenen op registers, en er
zijn er maar twee (`A` en `B`). Het **geheugen** is waar programma en data *wonen*: 256 bytes, maar de CPU rekent er
niet direct op. Om twee waardes uit het geheugen op te tellen, moet je ze eerst met `LOAD` in registers zetten.

Neem het moment direct na `STORE A 142`: `A` en `mem[142]` bevatten dezelfde waarde. `STORE` kopieert, het
verplaatst niet. Het verschil zit in wat je ermee kunt. `A` kan direct meedoen aan een berekening, `mem[142]`
bewaart alleen. In echte hardware zitten registers in de CPU zelf en zijn ze veel sneller dan RAM.

## 4. Wat gaat er mis als je `pc` niet verhoogt vóór je de volgende instructie fetcht?

- **Nooit verhogen:** `fetch()` leest steeds hetzelfde byte, dus de CPU voert eindeloos dezelfde instructie uit.
- **Na byte 1 niet verhogen (misalignment):** byte 2 wordt dan gelezen op dezelfde plek als byte 1. Bij
  `LOAD B 5` is de operand dan `00010000` (16) in plaats van 5. De volgende fetch leest byte 2 als opcode, en vanaf
  daar staat het hele programma een byte verschoven. Alle bytes zijn geldige patronen, dus er komt **geen
  foutmelding**: de CPU voert gewoon het verkeerde programma uit met verkeerde data. In mijn test werd een
  `ADD A B` zelfs als het getal 40 opgegeten door een `LOAD`.

De `pc` moet daarom elke keer worden verhoogd direct nadat je een byte ophaalt, vóórdat je met dat byte iets doet.

## 5. Hoe zou je dit uitbreiden naar 16-bit?

"16-bit" kan twee dingen betekenen, die je los kunt kiezen: de breedte van de **registers** (data) en van de
**adressen**. Hier groeien beide.

| Onderdeel | Nu (8-bit) | 16-bit |
|-----------|------------|--------|
| Registers `A`, `B` | `uint8_t`, 0 t/m 255 | `uint16_t`, 0 t/m 65.535 |
| `pc` | `uint8_t`, 256 adressen | `uint16_t`, 65.536 adressen (64 KiB) |
| Adres in byte 2 | 1 byte | 2 bytes: een adres past niet meer in één byte |
| Instructielengte | 2 bytes | 3 bytes (zelfde lengte voor alle instructies, met padding waar het extra byte niet nodig is) |
| `pc`-stap | +2 | +3 |
| Geheugen | `uint8_t mem[256]` | `uint8_t mem[65536]`, nog steeds bytes |

Wat er verder verandert:

- **`JMP`, `JZ`, `STORE`, `LOAD` (mode 1)** hebben het volle adres nodig. Ze halen twee bytes op en voegen die samen,
  bijvoorbeeld `(hoog << 8) | laag`. `ADD`, `SUB`, `PRINT` en `HALT` laten byte 2 en 3 als padding staan.
- **Bytevolgorde.** Elk adres bevat nog 1 byte, dus een 16-bit waarde (adres of registerinhoud) staat in twee
  opeenvolgende adressen. Je moet afspreken welke eerst komt. Ik kies **big-endian**: de hoogste byte op het lagere
  adres, dus `0x1234` wordt `0x12` op 142 en `0x34` op 143. (Little-endian, de laagste byte eerst, gebruiken x86 en
  de meeste ARM-systemen.)
- **`STORE` en `LOAD`** moeten bij 16-bit registers ook twee bytes schrijven en lezen.
- **Opcode, `dst`, `src` en mode** in byte 1 blijven gelijk, dus de `INSTR`-macro hoeft niet te veranderen.
- **Assembler:** getallen tot 65.535, drie bytes per instructie (dus `addr += 3`), 16-bit labeladressen en een
  nieuwe grens tussen programma en data (bijvoorbeeld 32.768).
- **Rekenen** gaat modulo 65.536 in plaats van modulo 256.