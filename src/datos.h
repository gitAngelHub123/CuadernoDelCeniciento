// Cuaderno DS3 — datos del juego (zonas, hogueras, jefes, ruta principal y NPCs).
// © 2026 Angel. Nombres de lugares/NPCs: textos oficiales en español (es-ES) del juego.
// Flags: ID de evento del juego. Fuentes: SoulSplitter (jefes/hogueras), tabla The Grand Archives
// (NPCs), comprobados contra el ejecutable 1.15.2. Lo marcado VERIFICAR aún no se probó en partida.
#pragma once
#include <cstdint>

// ------------------------------------------------------------------ jefes
enum Jefe : uint32_t {
    J_IUDEX = 14000800, J_VORDT = 13000800, J_ARBOL = 13100800, J_SABIO = 13300850,
    J_VIGILANTES = 13300800, J_DIACONOS = 13500800, J_WOLNIR = 13800800, J_REY_DEMONIO = 13800830,
    J_PONTIFICE = 13700850, J_YHORM = 13900800, J_ALDRICH = 13700800, J_BAILARINA = 13000890,
    J_ARMADURA = 13010800, J_OCEIROS = 13000830, J_CAMPEON_GUNDYR = 14000830, J_PRINCIPES = 13410830,
    J_WYVERN = 13200800, J_REY_SIN_NOMBRE = 13200850, J_ALMA_CENIZAS = 14100800,
    J_FRIEDE = 14500800, J_CUSTODIO = 14500860, J_PRINCIPE_DEMONIO = 15000800,
    J_PENUMBRA = 15100800, J_MIDIR = 15100850, J_GAEL = 15110800,
};

// ------------------------------------------------------------------ zonas
struct Zona {
    const char* nombre;
    uint32_t hogueras[8];   // flags de hoguera encendida (0 = fin)
    bool dlc;
};

enum ZonaId {
    Z_CEMENTERIO, Z_SANTUARIO, Z_GRAN_MURO, Z_ASENTAMIENTO, Z_CAMINO, Z_FARRON, Z_CATEDRAL,
    Z_CATACUMBAS, Z_LAGO, Z_IRITHYLL, Z_MAZMORRA, Z_CAPITAL, Z_ANOR_LONDO, Z_CASTILLO,
    Z_JARDIN, Z_ARCHIVO, Z_TUMBAS, Z_PICO, Z_HORNO, Z_ARIANDEL, Z_RESIDUOS, Z_ANILLADA,
    Z_TOTAL, Z_ARENA = 100, Z_DESCONOCIDA = -1
};

static const Zona ZONAS[Z_TOTAL] = {
    {u8"Cementerio de Ceniza",            {14000001, 14000002}, false},
    {u8"Santuario de Enlace del Fuego",   {14000000}, false},
    {u8"Gran muro de Lothric",            {13000009, 13000005, 13000002, 13000004}, false},
    {u8"Asentamiento de no muertos",      {13100004, 13100000, 13100002, 13100003, 13100001}, false},
    {u8"Camino de los Sacrificios",       {13300006, 13300000, 13300007, 13300002}, false},
    {u8"Torreón de Farron",               {13300003, 13300004, 13300008, 13300005, 13300001}, false},
    {u8"Catedral de la Oscuridad",        {13500003, 13500000, 13500002, 13500001}, false},
    {u8"Catacumbas de Carthus",           {13800006, 13800000, 13800001}, false},
    {u8"Lago Ardiente",                   {13800002, 13800003, 13800004}, false},
    {u8"Irithyll del Valle Boreal",       {13700007, 13700004, 13700000, 13700005, 13700001, 13700006, 13700008}, false},
    {u8"Mazmorra de Irithyll",            {13900000}, false},
    {u8"Capital Profanada",               {13900002, 13900001}, false},
    {u8"Anor Londo",                      {13700003, 13700002}, false},
    {u8"Castillo de Lothric",             {13010000, 13010002, 13010001}, false},
    {u8"Jardín del Rey Consumido",        {13000001}, false},
    {u8"Gran Archivo",                    {13410001, 13410000}, false},
    {u8"Tumbas Olvidadas",                {14000003, 14000004}, false},
    {u8"Pico del Archidragón",            {13200000, 13200003, 13200002, 13200001}, false},
    {u8"Horno de la Primera Llama",       {14100000, 14100001}, false},
    {u8"Mundo Pintado de Ariandel",       {14500001, 14500002, 14500003, 14500004, 14500005, 14500000, 14500007, 14500006}, true},
    {u8"El montón de residuos",           {15000001, 15000002, 15000003, 15000000}, true},
    {u8"La Ciudad Anillada",              {15100002, 15100003, 15100004, 15100005, 15100000, 15100001, 15110001, 15110000}, true},
};

// Jefes en orden aproximado de la historia (nombres oficiales es-ES)
struct JefeInfo { uint32_t flag; const char* nombre; bool dlc; int zona; };
static const JefeInfo JEFES[] = {
    {J_IUDEX, u8"Iudex Gundyr", false, Z_CEMENTERIO},                {J_VORDT, u8"Vordt del Valle Boreal", false, Z_GRAN_MURO},
    {J_ARBOL, u8"Gran Árbol corrompido", false, Z_ASENTAMIENTO},       {J_SABIO, u8"Sabio de cristal", false, Z_CAMINO},
    {J_DIACONOS, u8"Diáconos de la Oscuridad", false, Z_CATEDRAL}, {J_VIGILANTES, u8"Vigilantes del Abismo", false, Z_FARRON},
    {J_WOLNIR, u8"Gran Señor Wolnir", false, Z_CATACUMBAS},          {J_REY_DEMONIO, u8"Viejo rey demonio", false, Z_LAGO},
    {J_PONTIFICE, u8"Pontífice Sulyvahn", false, Z_IRITHYLL},      {J_YHORM, u8"Yhorm, el Gigante", false, Z_CAPITAL},
    {J_ALDRICH, u8"Aldrich, el Devoradioses", false, Z_ANOR_LONDO},  {J_BAILARINA, u8"Bailarina del Valle Boreal", false, Z_GRAN_MURO},
    {J_ARMADURA, u8"Armadura del Asesino de dragones", false, Z_CASTILLO}, {J_OCEIROS, u8"Oceiros, el Rey Consumido", false, Z_JARDIN},
    {J_CAMPEON_GUNDYR, u8"Campeón Gundyr", false, Z_TUMBAS},     {J_PRINCIPES, u8"Príncipes Gemelos", false, Z_ARCHIVO},
    {J_WYVERN, u8"Wyvern antiguo", false, Z_PICO},             {J_REY_SIN_NOMBRE, u8"Rey Sin Nombre", false, Z_PICO},
    {J_ALMA_CENIZAS, u8"Alma de Cenizas", false, Z_HORNO},
    {J_CUSTODIO, u8"Custodio de la tumba del Campeón", true, Z_ARIANDEL}, {J_FRIEDE, u8"Hermana Friede", true, Z_ARIANDEL},
    {J_PRINCIPE_DEMONIO, u8"Príncipe demonio", true, Z_RESIDUOS},  {J_PENUMBRA, u8"Penumbra, Lanza de la iglesia", true, Z_ANILLADA},
    {J_MIDIR, u8"Midir, el devorador de oscuridad", true, Z_ANILLADA}, {J_GAEL, u8"Caballero esclavo Gael", true, Z_ANILLADA},
};

// playRegionId ([[WorldChrMan]+80]+1ABC) -> zona. Rangos según la tabla de TGA; los ID que
// no aparezcan aquí se escriben en el registro para revisarlos.
static int zonaDeRegion(int r) {
    if (r <= 0) return Z_DESCONOCIDA;
    int area = r / 10000, sub = r % 10000;
    switch (area) {
    case 30: return (sub >= 20 && sub < 100) ? Z_JARDIN : Z_GRAN_MURO;
    case 31: return Z_ASENTAMIENTO;
    case 32: return Z_PICO;
    case 33: return (sub >= 10 && sub < 20) ? Z_FARRON : Z_CAMINO;
    case 34: return Z_ARCHIVO;
    case 35: return Z_CATEDRAL;
    case 37: return (sub >= 11 && sub <= 13) ? Z_ANOR_LONDO : Z_IRITHYLL;
    case 38: return (sub >= 10 && sub < 20) ? Z_LAGO : Z_CATACUMBAS;
    case 39: return (sub >= 3) ? Z_CAPITAL : Z_MAZMORRA;
    case 40: return (sub >= 100) ? (sub == 102 ? Z_SANTUARIO : Z_CEMENTERIO) : Z_TUMBAS;
    case 41: return Z_HORNO;
    case 45: return Z_ARIANDEL;
    case 46: case 47: case 53: case 54: return Z_ARENA;
    case 50: return Z_RESIDUOS;
    case 51: return Z_ANILLADA;
    default:
        if (r / 1000 == 301) return Z_CASTILLO;   // 301xxx
        if (r / 1000 == 341) return Z_ARCHIVO;
        if (r / 1000 == 511) return Z_ANILLADA;
        return Z_DESCONOCIDA;
    }
}
// 301xxx (Castillo) y 341xxx (Archivo) caen en area 30/34 con sub>=1000:
static int zonaDeRegionCompleta(int r) {
    if (r / 1000 == 301) return Z_CASTILLO;
    if (r / 1000 == 341) return Z_ARCHIVO;
    if (r / 1000 == 300) return (r % 1000 >= 20) ? Z_JARDIN : Z_GRAN_MURO;
    return zonaDeRegion(r);
}

// ------------------------------------------------------------------ NPCs
// Flags "en bruto" (desplazamiento dentro del bloque de flags + bit, igual que la tabla TGA).
struct FlagBruto { uint32_t off; uint8_t bit; };
static constexpr FlagBruto SIN_FLAG{0, 0xFF};

struct Paso {
    uint32_t requiere;     // flag (jefe/hoguera) que desbloquea este paso; 0 = desde el inicio
    const char* texto;     // nivel "Pistas": adónde ir y qué evitar, sin finales
};
struct Conexion {
    int otro;              // índice en NPCS
    const char* conNombre; // se muestra si conoces a los dos
    const char* anonimo;   // se muestra si aún no conoces al otro
};
struct Npc {
    const char* nombre;
    int prefijoDialogo;    // TalkID / 1000000 (0 = no habla)
    FlagBruto hablado;     // "has hablado con él en el santuario" (TGA), o SIN_FLAG
    FlagBruto enSantuario; // está en el Santuario (indica que ya lo conociste)
    FlagBruto muerto;      // VERIFICAR algunos
    Paso pasos[8];
    Conexion conexiones[3];
};

enum NpcId { N_SIRRIS, N_ANRI, N_HORACE, N_GREIRAT, N_SIEGWARD, N_IRINA, N_EYGON, N_YOEL, N_YURIA, N_PATCHES,
             N_GUARDIANA, N_SIRVIENTA, N_ANDRE, N_LUDLETH, N_HAWKWOOD, N_LEONHARD, N_ORBECK, N_CORNYX, N_KARLA,
             N_YORSHKA, N_EMMA, N_ANIDADO, N_PINTORA, N_GAEL, N_LAPP, N_SHIRA, N_ARPIA, N_TOTAL };

// Textos escritos a partir de guías de la comunidad (Fextralife). Pendientes de verificar en partida.
static const Npc NPCS[N_TOTAL] = {
    { u8"Sirris de los Reinos sin sol", 8, {0x2EE5E, 1}, {0x8C, 6}, {0x92, 5},
      { {0,          u8"Habla con ella en el Santuario y escucha lo que busca."},
        {J_SABIO,    u8"Busca sus «Cenizas de soñador» y llévalas a la Sirvienta. Luego vuelve a hablar con ella."},
        {13700004,   u8"En Irithyll, cerca del puente inicial, busca su señal de invocación y ayúdala."},
        {J_ALDRICH,  u8"Busca su señal junto a la niebla del Gran Árbol corrompido (Asentamiento)."},
        {J_PRINCIPES,u8"Vuelve al Santuario y a su tumba, entre el Santuario y la hoguera de Iudex."} },
      { {-1, u8"No ofrezcas Lenguas pálidas en el pacto de Rosaria: Sirris se marcharía.",
              u8"Ofrecer Lenguas pálidas en cierto pacto de la Catedral rompe esta misión."} } },

    { u8"Anri de Astora", 19, {0x2EE54, 3}, {0xA9, 1}, {0xAF, 1},
      { {0,           u8"La encontrarás junto a la hoguera de la Fortaleza del camino. Agota su conversación."},
        {J_DIACONOS,  u8"Búscala en el Santuario; después seguirá su camino hacia las Catacumbas de Carthus."},
        {13800006,    u8"En las Catacumbas está en dos sitios apartados del camino; habla con ella en ambos."},
        {J_WOLNIR,    u8"Más adelante la verás en la Iglesia de Yorshka (Irithyll)."},
        {J_PONTIFICE, u8"Tras el Pontífice, mantente atento a su señal o a noticias sobre ella en Anor Londo."} },
      { {N_HORACE, u8"Ligada a Horace: no le digas a Anri dónde está Horace mientras él siga vivo.",
                   u8"Cuidado: el destino de Anri depende de alguien que aún no has conocido."},
        {N_YURIA,  u8"Ligada a Yuria: en la Iglesia de Yorshka hay un peregrino; lo que hagas con él afecta a ambas misiones.",
                   u8"Cuidado: lo que hagas en la Iglesia de Yorshka afecta a otra misión que aún no conoces."} } },

    { u8"Horace el mudo", 0, SIN_FLAG, {0xBF, 5}, {0xBD, 5},
      { {0,          u8"Acompaña a Anri. No habla, pero su camino continúa en las Catacumbas."},
        {13800006,   u8"Más adelante estará en el Lago Ardiente, en un hueco de la pared derecha."} },
      { {N_ANRI, u8"Ligado a Anri: lo que pase con Horace decide el destino de Anri.", nullptr} } },

    { u8"Greirat del Asentamiento de no muertos", 12, SIN_FLAG, {0x9A, 6}, {0x98, 5},
      { {0,          u8"Está preso en el Gran muro: busca la llave de su celda bajo la Torre del muro."},
        {13100000,   u8"En el Asentamiento hay una casa oscura con un cadáver en el balcón: tráele lo que lleva."},
        {13100000,   u8"Puedes enviarlo a saquear. Vuelve con botín cuando avances (visita la zona o derrota a un jefe)."},
        {13700007,   u8"Ya puede ir a Irithyll. Ese viaje es peligroso: asegúrate de tener ayuda antes de enviarlo."} },
      { {N_PATCHES,  u8"Ligado a Patches: cuando Greirat esté en Irithyll, hablar con Patches puede salvarlo.",
                     u8"Cuidado: el éxito de su viaje a Irithyll depende de alguien que aún no has conocido."},
        {N_SIEGWARD, u8"Ligado a Siegward: mientras Siegward siga en Irithyll, Greirat corre menos peligro.",
                     u8"Cuidado: otra persona que viaja por Irithyll puede influir en su suerte."},
        {-1, u8"No lo envíes al Castillo de Lothric si quieres conservarlo.", nullptr} } },

    { u8"Siegward de Catarina", 21, SIN_FLAG, SIN_FLAG, SIN_FLAG,
      { {0,           u8"Habla con él en el ascensor del Asentamiento y vuelve a encontrarlo arriba en la torre."},
        {13500003,    u8"En la Catedral de la Oscuridad, cerca de la Capilla de purificación, busca un pozo."},
        {13700007,    u8"En Irithyll, sigue el agua: hay una chimenea junto a la Mansión lejana."},
        {13900000,    u8"Busca una llave en las cloacas de la Mazmorra; en la Capital Profanada hay una celda."},
        {J_YHORM,     u8"Habla con él después de Yhorm, y luego acércate al trono."} },
      { {-1, u8"Si saltas un encuentro, no te acompañará contra Yhorm.", nullptr} } },

    { u8"Irina de Carim", 16, SIN_FLAG, {0xA0, 6}, {0xA6, 5},
      { {0,          u8"Está encerrada en el Asentamiento, cerca de la salida al Camino de los Sacrificios."},
        {14000000,   u8"En el Santuario, llévale Tomos divinos en braille para que enseñe milagros."} },
      { {N_EYGON, u8"Ligada a Eygon: si le compras milagros oscuros, Eygon intervendrá.",
                  u8"Cuidado: comprarle milagros oscuros molestará a alguien que aún no conoces."} } },

    { u8"Eygon de Carim", 17, {0x2EE52, 5}, {0xA6, 1}, {0xA4, 1},
      { {0,          u8"Protege a Irina. Lo verás en el Asentamiento y después en el Santuario."} },
      { {N_IRINA, u8"Ligado a Irina: su actitud depende de lo que hagas con ella.",
                  u8"Cuidado: su actitud depende de cómo trates a alguien que aún no has conocido."} } },

    { u8"Yoel de Londor", 5, SIN_FLAG, {0x84, 2}, {0x8A, 1},
      { {0,          u8"Está junto a la hoguera A los pies del muro. Puede acompañarte al Santuario."},
        {14000000,   u8"En el Santuario puede «sacar tu verdadero poder» (Sello oscuro). Termina su historia antes de las Catacumbas."} },
      { {N_YURIA, u8"Ligado a Yuria: completar a Yoel antes de las Catacumbas abre otra misión.",
                  u8"Cuidado: su historia abre otra misión con alguien que aún no has conocido."} } },

    { u8"Yuria de Londor", 4, {0x2EE48, 7}, {0x86, 6}, {0x84, 5},
      { {0,           u8"Habla con ella en el Santuario."},
        {J_PONTIFICE, u8"Tras el Pontífice, vuelve a hablar con ella: te indicará adónde ir en Anor Londo."} },
      { {N_ANRI,  u8"Ligada a Anri: necesitas que Anri siga viva y no matar al peregrino de la Iglesia de Yorshka.",
                  u8"Cuidado: su misión depende de alguien que aún no has conocido."},
        {-1,      u8"Curar el Sello oscuro hace que se marche.", nullptr} } },

    { u8"El Irrompible Parches", 20, SIN_FLAG, {0xAE, 2}, {0xAC, 5},
      { {0,           u8"Desconfía de él. Lo verás varias veces; tarde o temprano acabará en el Santuario."} },
      { {N_GREIRAT,   u8"Ligado a Greirat: puede ayudarle durante su viaje a Irithyll.",
                      u8"Puede ser útil para otra persona que conocerás más adelante."},
        {N_SIEGWARD,  u8"Ligado a Siegward: tiene algo que Siegward necesita en la Catedral.",
                      u8"Tiene algo que alguien necesitará en la Catedral de la Oscuridad."} } },

    { u8"Guardiana del Fuego", 2, SIN_FLAG, SIN_FLAG, SIN_FLAG,
      { {0, u8"En el Santuario: entrégale almas para subir de nivel."} }, {} },
    { u8"Sirvienta del Santuario", 11, SIN_FLAG, SIN_FLAG, SIN_FLAG,
      { {0, u8"Vende objetos. Llévale las cenizas que encuentres para ampliar su tienda."} }, {} },
    { u8"Herrero Andre", 10, SIN_FLAG, SIN_FLAG, {0x95, 7},
      { {0, u8"Refuerza tus armas. Llévale los carbones que encuentres para nuevas infusiones."} }, {} },
    { u8"Ludleth de Courland", 3, SIN_FLAG, SIN_FLAG, {0x87, 1},
      { {0, u8"Desde su trono puede transponer almas de jefes cuando consigas cierto horno."} }, {} },
    { u8"Hawkwood, el desertor", 7, SIN_FLAG, SIN_FLAG, {0x8D, 1},
      { {0, u8"Habla con él de vez en cuando, sobre todo cuando derrotes a un Señor de la Ceniza."} }, {} },
    { u8"Leonhard, el Dedo Anular", 9, {0x2EE69, 5}, SIN_FLAG, {0x90, 1},
      { {0, u8"Agota su conversación en el Santuario; más adelante lo verás en otros lugares."} }, {} },
    { u8"Orbeck de Vinheim", 13, SIN_FLAG, {0x98, 2}, {0x9E, 1},
      { {0, u8"Hechicero: en el Santuario te enseña hechicerías. Llévale los pergaminos que encuentres."} }, {} },
    { u8"Cornyx del Gran Pantano", 14, SIN_FLAG, {0x9D, 6}, {0xA3, 5},
      { {0, u8"Piromántico: en el Santuario te enseña piromancias. Llévale los tomos de piromancia."} }, {} },
    { u8"Karla", 15, SIN_FLAG, {0xA3, 2}, {0xA1, 1},
      { {0, u8"Bruja: en el Santuario enseña hechizos oscuros. Llévale los tomos que encuentres."} }, {} },

    // ---- añadidos (base y DLC). Prefijos de diálogo identificados leyendo el texto del juego.
    { u8"Yorshka, Capitana de la compañía", 6, SIN_FLAG, SIN_FLAG, SIN_FLAG,
      { {0, u8"En lo alto de la Iglesia de Yorshka. Te ofrece su pacto; habla con ella de vez en cuando."} },
      { {N_SIRRIS, u8"Ligada a Sirris: no la ataques si quieres seguir la historia de Sirris.",
                   u8"Cuidado: atacarla puede afectar a otra misión que aún no conoces."} } },
    { u8"Emma, Suma sacerdotisa", 23, SIN_FLAG, SIN_FLAG, SIN_FLAG,
      { {0, u8"En la capilla del Gran muro. Vuelve con ella cuando avances en tu viaje."} }, {} },
    { u8"Anidado", 29, SIN_FLAG, SIN_FLAG, SIN_FLAG,
      { {0, u8"Un nido en un tejado cerca del Santuario: deja objetos y recibirás otros a cambio."} }, {} },
    { u8"La pintora", 53, SIN_FLAG, SIN_FLAG, SIN_FLAG,
      { {0, u8"En el Mundo Pintado de Ariandel. Escúchala; su historia continúa más adelante."} },
      { {N_GAEL, u8"Ligada a Gael: él le prometió algo para su cuadro.", u8"Su historia depende de alguien que aún no has conocido."} } },
    { u8"Caballero esclavo Gael", 71, SIN_FLAG, SIN_FLAG, SIN_FLAG,
      { {0, u8"Busca lo que la pintora necesita. Su camino continúa en la Ciudad Anillada."} },
      { {N_PINTORA, u8"Ligado a la pintora: busca algo para su cuadro.", nullptr} } },
    { u8"Lapp, el amnésico", 73, SIN_FLAG, SIN_FLAG, SIN_FLAG,
      { {0, u8"Lo encontrarás varias veces en la Ciudad Anillada; habla con él cada vez que lo veas."} }, {} },
    { u8"Shira, mujer cab. de Filianore", 74, SIN_FLAG, SIN_FLAG, SIN_FLAG,
      { {0, u8"Custodia de la Iglesia de Filianore. Agota su conversación."} }, {} },
    { u8"Vieja arpía de piedra", 76, SIN_FLAG, SIN_FLAG, SIN_FLAG,
      { {0, u8"Vende objetos en la Ciudad Anillada."} }, {} },
};

// Segundo prefijo de diálogo (algunos NPCs hablan con dos rangos) y bloque de 20 flags de cada NPC
// (x18 = muerto). Bloques: TGA y eventos del randomizer de thefifthmatt; 0 = desconocido.
static inline int prefijo2(int n) { return n == N_PINTORA ? 72 : n == N_SHIRA ? 78 : 0; }
static inline int bloqueNpc(int n) {
    switch (n) {
    case N_SIRRIS: return 1120;  case N_ANRI: return 1340;     case N_HORACE: return 1480;  case N_GREIRAT: return 1200;
    case N_SIEGWARD: return 1380; case N_IRINA: return 1280;   case N_EYGON: return 1300;   case N_YOEL: return 1060;
    case N_YURIA: return 1040;   case N_PATCHES: return 1360;  case N_ANDRE: return 1160;   case N_LUDLETH: return 1020;
    case N_HAWKWOOD: return 1100; case N_LEONHARD: return 1140; case N_ORBECK: return 1220; case N_CORNYX: return 1240;
    case N_KARLA: return 1260;
    default: return 0;
    }
}

// ------------------------------------------------------------------ objetos clave
// tipo: 0 arma, 1 armadura, 2 anillo, 4 objeto. "para": historia principal, misión de un NPC o utilidad.
struct Objeto { uint32_t id; uint8_t tipo; const char* nombre; const char* para; const char* uso; };
static const Objeto OBJETOS[] = {
    {2137, 4, u8"Espada en espiral", u8"Historia principal", u8"Clávala en la hoguera del Santuario para viajar entre hogueras."},
    {2102, 4, u8"Estandarte peq. de Lothric", u8"Historia principal", u8"Álzalo en la puerta principal del castillo (Gran muro) para llegar al Asentamiento."},
    {2005, 4, u8"Muñeco pequeño", u8"Historia principal", u8"Te permite cruzar la barrera de entrada a Irithyll."},
    {6370000, 0, u8"Soberano de las tormentas", u8"Historia principal", u8"Arma clave contra un jefe gigante de la Capital Profanada."},
    {2117, 4, u8"Jofaina de votos", u8"Historia principal", u8"Colócala en la estatua del caballero decapitado del Gran muro."},
    {2014, 4, u8"Llave del Gran Archivo", u8"Historia principal", u8"Abre la entrada principal del Gran Archivo."},
    {2123, 4, u8"Cenizas de un Señor (Vigilantes)", u8"Historia principal", u8"Llévalas a su trono en el Santuario."},
    {2124, 4, u8"Cenizas de un Señor (Aldrich)", u8"Historia principal", u8"Llévalas a su trono en el Santuario."},
    {2125, 4, u8"Cenizas de un Señor (Yhorm)", u8"Historia principal", u8"Llévalas a su trono en el Santuario."},
    {2126, 4, u8"Cenizas de un Señor (Lothric)", u8"Historia principal", u8"Llévalas a su trono en el Santuario."},
    {9030, 4, u8"Camino del dragón", u8"Zona opcional", u8"Gesto que abre el camino al Pico del Archidragón."},
    {2156, 4, u8"Estandarte pequeño del Enviado", u8"Contenido descargable", u8"Sostenlo ante el acantilado de la Ciudad Anillada."},
    {2155, 4, u8"Llave de mecanismo", u8"Contenido descargable", u8"Da acceso al desván de una biblioteca."},
    {2010, 4, u8"Llave de celda", u8"Misión de Greirat", u8"Abre la celda donde está preso Greirat en el Gran muro."},
    {2118, 4, u8"Hueso de Loretta", u8"Misión de Greirat", u8"Llévaselo a Greirat."},
    {2108, 4, u8"Cenizas de soñador", u8"Misión de Sirris", u8"Entrégaselas a la Sirvienta del Santuario."},
    {2008, 4, u8"Llavero del carcelero", u8"Misión de Karla", u8"Abre celdas de la Mazmorra de Irithyll, entre ellas la de Karla."},
    {2012, 4, u8"Llave de celda vieja", u8"Misión de Siegward", u8"Abre la celda más vieja: alguien conocido espera allí."},
    {10000, 1, u8"Yelmo de Catarina", u8"Misión de Siegward", u8"Parte de la armadura que Siegward necesita."},
    {11000, 1, u8"Armadura de Catarina", u8"Misión de Siegward", u8"Parte de la armadura que Siegward necesita."},
    {12000, 1, u8"Guanteletes de Catarina", u8"Misión de Siegward", u8"Parte de la armadura que Siegward necesita."},
    {13000, 1, u8"Polainas de Catarina", u8"Misión de Siegward", u8"Parte de la armadura que Siegward necesita."},
    {21140000, 0, u8"Escudo perforador", u8"Misión de Siegward", u8"Recompensa ligada a la historia de Siegward."},
    {2119, 4, u8"Tomo divino de Carim en braille", u8"Misión de Irina", u8"Dáselo a Irina para que enseñe sus milagros."},
    {2120, 4, u8"Tomo divino de Lothric en braille", u8"Misión de Irina", u8"Dáselo a Irina para que enseñe sus milagros."},
    {2144, 4, u8"Tomo divino de Oscuridad en braille", u8"Misión de Irina o Karla", u8"Milagros oscuros: piénsalo antes de dárselo a Irina."},
    {2145, 4, u8"Tomo divino de Londor en braille", u8"Misión de Irina o Karla", u8"Milagros oscuros: piénsalo antes de dárselo a Irina."},
    {2127, 4, u8"Tomo de pirom. del Gran Pantano", u8"Misión de Cornyx", u8"Dáselo a Cornyx para aprender piromancias."},
    {2128, 4, u8"Tomo de piromancia de Carthus", u8"Misión de Cornyx", u8"Dáselo a Cornyx para aprender piromancias."},
    {2129, 4, u8"Tomo de piromancia de Izalith", u8"Misión de Cornyx", u8"Dáselo a Cornyx para aprender piromancias."},
    {2130, 4, u8"Tomo de piromancia de Quelana", u8"Misión de Karla", u8"Dáselo a Karla para aprender piromancias."},
    {2131, 4, u8"Tomo de pirom. del guardián de la cripta", u8"Misión de Karla", u8"Dáselo a Karla para aprender piromancias de llama negra."},
    {2132, 4, u8"Pergamino de Sabio", u8"Misión de Orbeck", u8"Dáselo a Orbeck para aprender hechizos."},
    {2133, 4, u8"Pergamino de Logan", u8"Misión de Orbeck", u8"Dáselo a Orbeck para aprender hechizos."},
    {2134, 4, u8"Pergamino de cristal", u8"Misión de Orbeck", u8"Dáselo a Orbeck para aprender hechizos."},
    {2140, 4, u8"Pergamino dorado", u8"Misión de Orbeck", u8"Dáselo a Orbeck para aprender hechizos."},
    {2135, 4, u8"Horno de transposición", u8"Ludleth", u8"Dáselo a Ludleth para transponer almas de jefes."},
    {490, 4, u8"Marca oscura", u8"Misión de Yoel y Yuria", u8"Acumula maldición. Curarla cambia el rumbo de esa historia."},
    {2139, 4, u8"Espada del manifiesto", u8"Misión de Yuria y Anri", u8"Ligada a una ceremonia en Anor Londo."},
    {728, 4, u8"Alma de Rosaria", u8"Misión de Leonhard", u8"Ligada a Rosaria y a quien la busca."},
    {115, 4, u8"Orbe del Ojo Negro", u8"Misión de Leonhard", u8"Invade el mundo de quien hizo daño a Rosaria."},
    {388, 4, u8"Piedra de torso de dragón centelleante", u8"Misión de Hawkwood", u8"Recompensa ligada a la historia de Hawkwood."},
    {2138, 4, u8"Ojos de Guardiana del Fuego", u8"Guardiana del Fuego", u8"Ligados a la Guardiana del Fuego y al final del juego."},
    {390, 4, u8"Alma de Guardiana del Fuego", u8"Guardiana del Fuego", u8"Dásela a la Guardiana del Fuego."},
    {2009, 4, u8"Llave de cripta", u8"Utilidad (ruta a Irina)", u8"Abre las cloacas del Asentamiento."},
    {2001, 4, u8"Llave de la sala del elevador", u8"Utilidad", u8"Abre un elevador subterráneo."},
    {2013, 4, u8"Llave de la Torre", u8"Utilidad", u8"Abre la torre ruinosa tras el Santuario."},
    {2007, 4, u8"Llave del fugado", u8"Utilidad", u8"Abre la ventana de una celda de la Mazmorra de Irithyll."},
    {2107, 4, u8"Cenizas de enterrador", u8"Sirvienta del Santuario", u8"Dáselas a la Sirvienta: ampliará su tienda."},
};

// ------------------------------------------------------------------ coleccionables (paso 4)
// Objetos que se entregan a alguien para desbloquear cosas. destino: índice en DESTINOS.
static const char* DESTINOS[] = {u8"la Sirvienta del Santuario", u8"el herrero Andre", u8"Irina", u8"Cornyx", u8"Karla", u8"Orbeck"};
static const char* DESBLOQUEA[] = {u8"amplía su tienda", u8"nuevas infusiones para tus armas", u8"nuevos milagros",
                                   u8"nuevas piromancias", u8"nuevos hechizos", u8"nuevas hechicerías"};
struct Coleccionable { uint32_t id; uint8_t destino; const char* nombre; };
static const Coleccionable COLECCIONABLES[] = {
    {2107, 0, u8"Cenizas de enterrador"}, {2108, 0, u8"Cenizas de soñador"}, {2109, 0, u8"Cenizas de paladín"},
    {2110, 0, u8"Cenizas guardián de cripta"}, {2146, 0, u8"Cenizas con excrementos"}, {2147, 0, u8"Cenizas jefe de prisioneros"},
    {2148, 0, u8"Cenizas de Xanthous"}, {2149, 0, u8"Cenizas de Hueco"}, {2151, 0, u8"Cenizas cazador de dragones"},
    {2152, 0, u8"Cenizas habitante del este"}, {2154, 0, u8"Cenizas del Capitán"}, {2157, 0, u8"Cenizas de anciana"},
    {2111, 0, u8"Cenizas de Greirat"}, {2112, 0, u8"Cenizas de Orbeck"}, {2113, 0, u8"Cenizas de Cornyx"},
    {2114, 0, u8"Cenizas de Karla"}, {2115, 0, u8"Cenizas de Irina"}, {2116, 0, u8"Cenizas de Yuria"},
    {2150, 0, u8"Cenizas de Parches"},
    {2103, 1, u8"Carbón de Farron"}, {2104, 1, u8"Carbón de sabio"}, {2105, 1, u8"Carbón de gigante"}, {2106, 1, u8"Carbón profanado"},
    {2119, 2, u8"Tomo divino de Carim en braille"}, {2120, 2, u8"Tomo divino de Lothric en braille"},
    {2144, 2, u8"Tomo divino de Oscuridad en braille"}, {2145, 2, u8"Tomo divino de Londor en braille"},
    {2127, 3, u8"Tomo de pirom. del Gran Pantano"}, {2128, 3, u8"Tomo de piromancia de Carthus"}, {2129, 3, u8"Tomo de piromancia de Izalith"},
    {2130, 4, u8"Tomo de piromancia de Quelana"}, {2131, 4, u8"Tomo de pirom. del guardián de la cripta"},
    {2132, 5, u8"Pergamino de Sabio"}, {2133, 5, u8"Pergamino de Logan"}, {2134, 5, u8"Pergamino de cristal"}, {2140, 5, u8"Pergamino dorado"},
};

// Bloque de flags de NPCs a vigilar en el registro de diagnóstico (flags 1000–1999).
static constexpr uint32_t DIAG_DESDE = 0x80, DIAG_HASTA = 0x100;
