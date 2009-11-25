//==================================================================//
/*
    AtomicParsley - AP_arrays.cpp

    AtomicParsley is GPL software; you can freely distribute, 
    redistribute, modify & use under the terms of the GNU General
    Public License; either version 2 or its successor.

    AtomicParsley is distributed under the GPL "AS IS", without
    any warranty; without the implied warranty of merchantability
    or fitness for either an expressed or implied particular purpose.

    Please see the included GNU General Public License (GPL) for 
    your rights and further details; see the file COPYING. If you
    cannot, write to the Free Software Foundation, 59 Temple Place
    Suite 330, Boston, MA 02111-1307, USA.  Or www.fsf.org

    Copyright ©2005-2007 puck_lock
		
		----------------------
    Code Contributions by:
		
    * Mellow_Flow - fix genre matching/verify genre limits
                                                                   */
//==================================================================//

//#include <sys/types.h>
//#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "AP_commons.h"     //just so win32/msvc can get uint8_t defined
#include "AtomicParsley.h"  //for stiks & sfIDs

//////////////

static const char* ID3v1GenreList[] = {
    "Blues", "Classic Rock", "Country", "Dance", "Disco",
		"Funk", "Grunge", "Hip-Hop", "Jazz", "Metal",
		"New Age", "Oldies", "Other", "Pop", "R&B",
		"Rap", "Reggae", "Rock", "Techno", "Industrial",
		"Alternative", "Ska", "Death Metal", "Pranks", "Soundtrack",
		"Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
    "Fusion", "Trance", "Classical", "Instrumental", "Acid",
		"House", "Game", "Sound Clip", "Gospel", "Noise",
		"AlternRock", "Bass", "Soul", "Punk", "Space", 
		"Meditative", "Instrumental Pop", "Instrumental Rock", "Ethnic", "Gothic", 
		"Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk", "Eurodance", 
		"Dream", "Southern Rock", "Comedy", "Cult", "Gangsta",
		"Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American",
		"Cabaret", "New Wave", "Psychadelic", "Rave", "Showtunes",
		"Trailer", "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz",
		"Polka", "Retro", "Musical", "Rock & Roll", "Hard Rock",
		"Folk", "Folk/Rock", "National Folk", "Swing", "Fast Fusion",
		"Bebob", "Latin", "Revival", "Celtic", "Bluegrass",
		"Avantgarde", "Gothic Rock", "Progressive Rock", "Psychedelic Rock", "Symphonic Rock",
		"Slow Rock", "Big Band", "Chorus", "Easy Listening", "Acoustic", 
		"Humour", "Speech", "Chanson", "Opera", "Chamber Music", "Sonata", 
		"Symphony", "Booty Bass", "Primus", "Porn Groove", 
		"Satire", "Slow Jam", "Club", "Tango", "Samba", 
		"Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", 
		"Duet", "Punk Rock", "Drum Solo", "A Capella", "Euro-House",
		"Dance Hall" };
		/*
		"Goa", "Drum & Bass", "Club House", "Hardcore", 
		"Terror", "Indie", "BritPop", "NegerPunk", "Polsk Punk", 
		"Beat", "Christian Gangsta", "Heavy Metal", "Black Metal", "Crossover", 
		"Contemporary C", "Christian Rock", "Merengue", "Salsa", "Thrash Metal", 
		"Anime", "JPop", "SynthPop",
}; */  //apparently the other winamp id3v1 extensions aren't valid

stiks stikArray[] = {
	{ "Movie", 0 },
	{ "Normal", 1 },
	{ "Audiobook", 2 },
	{ "Whacked Bookmark", 5 },
	{ "Music Video", 6 },
	{ "Short Film", 9 },
	{ "TV Show", 10 },
	{ "Booklet", 11 }
};

// from William Herrera: http://search.cpan.org/src/BILLH/LWP-UserAgent-iTMS_Client-0.16/lib/LWP/UserAgent/iTMS_Client.pm
sfIDs storefronts[] = {
	{ "United States",	143441 },
	{ "France",					143442 },
	{ "Germany",				143443 },
	{ "United Kingdom",	143444 },
	{ "Austria",				143445 },
	{ "Belgium",				143446 },
	{ "Finland",				143447 },
	{ "Greece",					143448 },
	{ "Ireland",				143449 },
	{ "Italy",					143450 },
	{ "Luxembourg",			143451 },
	{ "Netherlands",		143452 },
	{ "Portugal",				143453 },
	{ "Spain",					143454 },
	{ "Canada",					143455 },
	{ "Sweden",					143456 },
	{ "Norway",					143457 },
	{ "Denmark",				143458 },
	{ "Switzerland",		143459 },
	{ "Australia",			143460 },
	{ "New Zealand",		143461 },
	{ "Japan",					143462 }
};

iso639_lang known_languages[] = {
	{ "aar",			"aa", 		"Afar" },
	{ "abk",			"ab",			"Abkhazian" },
	{ "ace",			NULL,			"Achinese" },
	{ "ach",			NULL,			"Acoli" },
	{ "ada",			NULL,			"Adangme" },
	{ "ady",			NULL,			"Adyghe; Adygei" },
	{ "afa",			NULL,			"Afro-Asiatic (Other)" },
	{ "afh",			NULL,			"Afrihili" },
	{ "afr",			"af",			"Afrikaans" },
	{ "ain",			NULL,			"Ainu" },
	{ "aka",			"ak",			"Akan" },
	{ "akk",			NULL,			"Akkadian" },
	{ "alb/sqi",	"sq",			"Albanian" }, //dual codes
	{ "ale",			NULL,			"Aleut" },
	{ "alg",			NULL,			"Algonquian languages" },
	{ "alt",			NULL,			"Southern Altai" },
	{ "amh",			"am",			"Amharic" },
	{ "ang",			NULL,			"English, Old (ca.450-1100)" },
	{ "anp",			NULL,			"Angika" },
	{ "apa",			NULL,			"Apache languages" },
	{ "ara",			"ar",			"Arabic" },
	{ "arc",			NULL,			"Aramaic" },
	{ "arg",			"an",			"Aragonese" },
	{ "arm/hye",	"hy",			"Armenian" }, //dual codes
	{ "arn",			NULL,			"Araucanian" },
	{ "arp",			NULL,			"Arapaho" },
	{ "art",			NULL,			"Artificial (Other)" },
	{ "arw",			NULL,			"Arawak" },
	{ "asm",			"as",			"Assamese" },
	{ "ast",			NULL,			"Asturian; Bable" },
	{ "ath",			NULL,			"Athapascan languages" },
	{ "aus",			NULL,			"Australian languages" },
	{ "ava",			"av",			"Avaric" },
	{ "ave",			"ae",			"Avestan" },
	{ "awa",			NULL,			"Awadhi" },
	{ "aym",			"ay",			"Aymara" },
	{ "aze",			"az",			"Azerbaijani" },
	{ "bad",			NULL,			"Banda" },
	{ "bai",			NULL,			"Bamileke languages" },
	{ "bak",			"ba",			"Bashkir" },
	{ "bal",			NULL,			"Baluchi" },
	{ "bam",			"bm",			"Bambara" },
	{ "ban",			NULL,			"Balinese" },
	{ "baq/eus",	"eu",			"Basque" }, //dual codes
	{ "bas",			NULL,			"Basa" },
	{ "bat",			NULL,			"Baltic (Other)" },
	{ "bej",			NULL,			"Beja" },
	{ "bel",			"be",			"Belarusian" },
	{ "bem",			NULL,			"Bemba" },
	{ "ben",			"bn",			"Bengali" },
	{ "ber",			NULL,			"Berber (Other)" },
	{ "bho",			NULL,			"Bhojpuri" },
	{ "bih",			"bh",			"Bihari" },
	{ "bik",			NULL,			"Bikol" },
	{ "bin",			NULL,			"Bini" },
	{ "bis",			"bi",			"Bislama" },
	{ "bla",			NULL,			"Siksika" },
	{ "bnt",			NULL,			"Bantu (Other)" },
	{ "bos",			"bs",			"Bosnian" },
	{ "bra",			NULL,			"Braj" },
	{ "bre",			"br",			"Breton" },
	{ "btk",			NULL,			"Batak (Indonesia)" },
	{ "bua",			NULL,			"Buriat" },
	{ "bug",			NULL,			"Buginese" },
	{ "bul",			"bg",			"Bulgarian" },
	{ "bur/mya",	"my",			"Burmese" }, //dual codes
	{ "byn",			NULL,			"Blin; Bilin" },
	{ "cad",			NULL,			"Caddo" },
	{ "cai",			NULL,			"Central American Indian (Other)" },
	{ "car",			NULL,			"Carib" },
	{ "cat",			"ca",			"Catalan; Valencian" },
	{ "cau",			NULL,			"Caucasian (Other)" },
	{ "ceb",			NULL,			"Cebuano" },
	{ "cel",			NULL,			"Celtic (Other)" },
	{ "cha",			"ch",			"Chamorro" },
	{ "chb",			NULL,			"Chibcha" },
	{ "che",			"ce",			"Chechen" },
	{ "chg",			NULL,			"Chagatai" },
	{ "chk",			NULL,			"Chuukese" },
	{ "chm",			NULL,			"Mari" },
	{ "chn",			NULL,			"Chinook jargon" },
	{ "cho",			NULL,			"Choctaw" },
	{ "chp",			NULL,			"Chipewyan" },
	{ "chr",			NULL,			"Cherokee" },
	{ "chu",			"cu",			"Church Slavic; Old Slavonic; Church Slavonic; Old Bulgarian; Old Church Slavonic" },
	{ "chv",			"cv",			"Chuvash" },
	{ "chy",			NULL,			"Cheyenne" },
	{ "cmc",			NULL,			"Chamic languages" },
	{ "cop",			NULL,			"Coptic" },
	{ "cor",			"kw",			"Cornish" },
	{ "cos",			"co",			"Corsican" },
	{ "cpe",			NULL,			"Creoles and pidgins, English based (Other)" },
	{ "cpf",			NULL,			"Creoles and pidgins, French-based (Other)" },
	{ "cpp",			NULL,			"Creoles and pidgins, Portuguese-based (Other)" },
	{ "cre",			"cr",			"Cree" },
	{ "crh",			NULL,			"Crimean Tatar; Crimean Turkish" },
	{ "crp",			NULL,			"Creoles and pidgins (Other)" },
	{ "csb",			NULL,			"Kashubian" },
	{ "cus",			NULL,			"Cushitic (Other)" },
	{ "cze/ces",	"cs",			"Czech" }, //dual codes
	{ "dak",			NULL,			"Dakota" },
	{ "dan",			"da",			"Danish" },
	{ "dar",			NULL,			"Dargwa" },
	{ "day",			NULL,			"Dayak" },
	{ "del",			NULL,			"Delaware" },
	{ "den",			NULL,			"Slave (Athapascan)" },
	{ "dgr",			NULL,			"Dogrib" },
	{ "din",			NULL,			"Dinka" },
	{ "div",			"dv",			"Divehi; Dhivehi; Maldivian" },
	{ "doi",			NULL,			"Dogri" },
	{ "dra",			NULL,			"Dravidian (Other)" },
	{ "dsb",			NULL,			"Lower Sorbian" },
	{ "dua",			NULL,			"Duala" },
	{ "dum",			NULL,			"Dutch, Middle (ca.1050-1350)" },
	{ "dut/nld",	"nl",			"Dutch; Flemish" }, //dual codes
	{ "dyu",			NULL,			"Dyula" },
	{ "dzo",			"dz",			"Dzongkha" },
	{ "efi",			NULL,			"Efik" },
	{ "egy",			NULL,			"Egyptian (Ancient)" },
	{ "eka",			NULL,			"Ekajuk" },
	{ "elx",			NULL,			"Elamite" },
	{ "eng",			"en",			"English" },
	{ "enm",			NULL,			"English, Middle (1100-1500)" },
	{ "epo",			"eo",			"Esperanto" },
	{ "est",			"et",			"Estonian" },
	{ "ewe",			"ee",			"Ewe" },
	{ "ewo",			NULL,			"Ewondo" },
	{ "fan",			NULL,			"Fang" },
	{ "fao",			"fo",			"Faroese" },
	{ "fat",			NULL,			"Fanti" },
	{ "fij",			"fj",			"Fijian" },
	{ "fil",			NULL,			"Filipino; Pilipino" },
	{ "fin",			"fi",			"Finnish" },
	{ "fiu",			NULL,			"Finno-Ugrian (Other)" },
	{ "fon",			NULL,			"Fon" },
	{ "fre/fra",	"fr",			"French" }, //dual codes
	{ "frm",			NULL,			"French, Middle (ca.1400-1600)" },
	{ "fro",			NULL,			"French, Old (842-ca.1400)" },
	{ "frr",			NULL,			"Northern Frisian" },
	{ "frs",			NULL,			"Eastern Frisian" },
	{ "fry",			"fy",			"Western Frisian" },
	{ "ful",			"ff",			"Fulah" },
	{ "fur",			NULL,			"Friulian" },
	{ "gaa",			NULL,			"Ga" },
	{ "gay",			NULL,			"Gayo" },
	{ "gba",			NULL,			"Gbaya" },
	{ "gem",			NULL,			"Germanic (Other)" },
	{ "geo/kat",	"ka",			"Georgian" }, //dual codes
	{ "ger/deu",	"de",			"German" },  //dual codes
	{ "gez",			NULL,			"Geez" },
	{ "gil",			NULL,			"Gilbertese" },
	{ "gla",			"gd",			"Gaelic; Scottish Gaelic" },
	{ "gle",			"ga",			"Irish" },
	{ "glg",			"gl",			"Galician" },
	{ "glv",			"gv",			"Manx" },
	{ "gmh",			NULL,			"German, Middle High (ca.1050-1500)" },
	{ "goh",			NULL,			"German, Old High (ca.750-1050)" },
	{ "gon",			NULL,			"Gondi" },
	{ "gor",			NULL,			"Gorontalo" },
	{ "got",			NULL,			"Gothic" },
	{ "grb",			NULL,			"Grebo" },
	{ "grc",			NULL,			"Greek, Ancient (to 1453)" },
	{ "gre/ell",	"el",			"Greek, Modern (1453-)" }, //dual codes
	{ "grn",			"gn",			"Guarani" },
	{ "gsw",			NULL,			"Alemanic; Swiss German" },
	{ "guj",			"gu",			"Gujarati" },
	{ "gwi",			NULL,			"Gwich«in" },
	{ "hai",			NULL,			"Haida" },
	{ "hat",			"ht",			"Haitian; Haitian Creole" },
	{ "hau",			"ha",			"Hausa" },
	{ "haw",			NULL,			"Hawaiian" },
	{ "heb",			"he",			"Hebrew" },
	{ "her",			"hz",			"Herero" },
	{ "hil",			NULL,			"Hiligaynon" },
	{ "him",			NULL,			"Himachali" },
	{ "hin",			"hi",			"Hindi" },
	{ "hit",			NULL,			"Hittite" },
	{ "hmn",			NULL,			"Hmong" },
	{ "hmo",			"ho",			"Hiri Motu" },
	{ "hsb",			NULL,			"Upper Sorbian" },
	{ "hun",			"hu",			"Hungarian" },
	{ "hup",			NULL,			"Hupa" },
	{ "arm/hye",	"hy",			"Armenian" },
	{ "iba",			NULL,			"Iban" },
	{ "ibo",			"ig",			"Igbo" },
	{ "ice/isl",	"is",			"Icelandic" }, //dual codes
	{ "ido",			"io",			"Ido" },
	{ "iii",			"ii",			"Sichuan Yi" },
	{ "ijo",			NULL,			"Ijo" },
	{ "iku",			"iu",			"Inuktitut" },
	{ "ile",			"ie",			"Interlingue" },
	{ "ilo",			NULL,			"Iloko" },
	{ "ina",			"ia",			"Interlingua (International Auxiliary, Language Association)" },
	{ "inc",			NULL,			"Indic (Other)" },
	{ "ind",			"id",			"Indonesian" },
	{ "ine",			NULL,			"Indo-European (Other)" },
	{ "inh",			NULL,			"Ingush" },
	{ "ipk",			"ik",			"Inupiaq" },
	{ "ira",			NULL,			"Iranian (Other)" },
	{ "iro",			NULL,			"Iroquoian languages" },
	{ "ita",			"it",			"Italian" },
	{ "jav",			"jv",			"Javanese" },
	{ "jbo",			NULL,			"Lojban" },
	{ "jpn",			"ja",			"Japanese" },
	{ "jpr",			NULL,			"Judeo-Persian" },
	{ "jrb",			NULL,			"Judeo-Arabic" },
	{ "kaa",			NULL,			"Kara-Kalpak" },
	{ "kab",			NULL,			"Kabyle" },
	{ "kac",			NULL,			"Kachin" },
	{ "kal",			"kl",			"Kalaallisut; Greenlandic" },
	{ "kam",			NULL,			"Kamba" },
	{ "kan",			"kn",			"Kannada" },
	{ "kar",			NULL,			"Karen" },
	{ "kas",			"ks",			"Kashmiri" },
	{ "kau",			"kr",			"Kanuri" },
	{ "kaw",			NULL,			"Kawi" },
	{ "kaz",			"kk",			"Kazakh" },
	{ "kbd",			NULL,			"Kabardian" },
	{ "kha",			NULL,			"Khasi" },
	{ "khi",			NULL,			"Khoisan (Other)" },
	{ "khm",			"km",			"Khmer" },
	{ "kho",			NULL,			"Khotanese" },
	{ "kik",			"ki",			"Kikuyu; Gikuyu" },
	{ "kin",			"rw",			"Kinyarwanda" },
	{ "kir",			"ky",			"Kirghiz" },
	{ "kmb",			NULL,			"Kimbundu" },
	{ "kok",			NULL,			"Konkani" },
	{ "kom",			"kv",			"Komi" },
	{ "kon",			"kg",			"Kongo" },
	{ "kor",			"ko",			"Korean" },
	{ "kos",			NULL,			"Kosraean" },
	{ "kpe",			NULL,			"Kpelle" },
	{ "krc",			NULL,			"Karachay-Balkar" },
	{ "krl",			NULL,			"Karelian" },
	{ "kro",			NULL,			"Kru" },
	{ "kru",			NULL,			"Kurukh" },
	{ "kua",			"kj",			"Kuanyama; Kwanyama" },
	{ "kum",			NULL,			"Kumyk" },
	{ "kur",			"ku",			"Kurdish" },
	{ "kut",			NULL,			"Kutenai" },
	{ "lad",			NULL,			"Ladino" },
	{ "lah",			NULL,			"Lahnda" },
	{ "lam",			NULL,			"Lamba" },
	{ "lao",			"lo",			"Lao" },
	{ "lat",			"la",			"Latin" },
	{ "lav",			"lv",			"Latvian" },
	{ "lez",			NULL,			"Lezghian" },
	{ "lim",			"li",			"Limburgan; Limburger; Limburgish" },
	{ "lin",			"ln",			"Lingala" },
	{ "lit",			"lt",			"Lithuanian" },
	{ "lol",			NULL,			"Mongo" },
	{ "loz",			NULL,			"Lozi" },
	{ "ltz",			"lb",			"Luxembourgish; Letzeburgesch" },
	{ "lua",			NULL,			"Luba-Lulua" },
	{ "lub",			"lu",			"Luba-Katanga" },
	{ "lug",			"lg",			"Ganda" },
	{ "lui",			NULL,			"Luiseno" },
	{ "lun",			NULL,			"Lunda" },
	{ "luo",			NULL,			"Luo (Kenya and Tanzania)" },
	{ "lus",			NULL,			"Lushai" },
	{ "mad",			NULL,			"Madurese" },
	{ "mag",			NULL,			"Magahi" },
	{ "mah",			"mh",			"Marshallese" },
	{ "mai",			NULL,			"Maithili" },
	{ "mak",			NULL,			"Makasar" },
	{ "mal",			"ml",			"Malayalam" },
	{ "man",			NULL,			"Mandingo" },
	{ "map",			NULL,			"Austronesian (Other)" },
	{ "mar",			"mr",			"Marathi" },
	{ "mas",			NULL,			"Masai" },
	{ "may/msa",	"ms",			"Malay" }, //dual codes
	{ "mdf",			NULL,			"Moksha" },
	{ "mdr",			NULL,			"Mandar" },
	{ "men",			NULL,			"Mende" },
	{ "mga",			NULL,			"Irish, Middle (900-1200)" },
	{ "mic",			NULL,			"Mi'kmaq; Micmac" },
	{ "min",			NULL,			"Minangkabau" },
	{ "mis",			NULL,			"Miscellaneous languages" },
	{ "mac/mkd",	"mk",			"Macedonian" }, //dual codes
	{ "mkh",			NULL,			"Mon-Khmer (Other)" },
	{ "mlg",			"mg",			"Malagasy" },
	{ "mlt",			"mt",			"Maltese" },
	{ "mnc",			NULL,			"Manchu" },
	{ "mni",			NULL,			"Manipuri" },
	{ "mno",			NULL,			"Manobo languages" },
	{ "moh",			NULL,			"Mohawk" },
	{ "mol",			"mo",			"Moldavian" },
	{ "mon",			"mn",			"Mongolian" },
	{ "mos",			NULL,			"Mossi" },
	{ "mao/mri",	"mi",			"Maori" }, //dual codes
	{ "mul",			NULL,			"Multiple languages" },
	{ "mun",			NULL,			"Munda languages" },
	{ "mus",			NULL,			"Creek" },
	{ "mwl",			NULL,			"Mirandese" },
	{ "mwr",			NULL,			"Marwari" },
	{ "myn",			NULL,			"Mayan languages" },
	{ "myv",			NULL,			"Erzya" },
	{ "nah",			NULL,			"Nahuatl" },
	{ "nai",			NULL,			"North American Indian" },
	{ "nap",			NULL,			"Neapolitan" },
	{ "nau",			"na",			"Nauru" },
	{ "nav",			"nv",			"Navajo; Navaho" },
	{ "nbl",			"nr",			"Ndebele, South; South Ndebele" },
	{ "nde",			"nd",			"Ndebele, North; North Ndebele" },
	{ "ndo",			"ng",			"Ndonga" },
	{ "nds",			NULL,			"Low German; Low Saxon; German, Low; Saxon, Low" },
	{ "nep",			"ne",			"Nepali" },
	{ "new",			NULL,			"Newari; Nepal Bhasa" },
	{ "nia",			NULL,			"Nias" },
	{ "nic",			NULL,			"Niger-Kordofanian (Other)" },
	{ "niu",			NULL,			"Niuean" },
	{ "nno",			"nn",			"Norwegian Nynorsk; Nynorsk, Norwegian" },
	{ "nob",			"nb",			"Norwegian BokmŒl; BokmŒl, Norwegian" },
	{ "nog",			NULL,			"Nogai" },
	{ "non",			NULL,			"Norse, Old" },
	{ "nor",			"no",			"Norwegian" },
	{ "nqo",			NULL,			"N'ko" },
	{ "nso",			NULL,			"Northern Sotho, Pedi; Sepedi" },
	{ "nub",			NULL,			"Nubian languages" },
	{ "nwc",			NULL,			"Classical Newari; Old Newari; Classical Nepal Bhasa" },
	{ "nya",			"ny",			"Chichewa; Chewa; Nyanja" },
	{ "nym",			NULL,			"Nyamwezi" },
	{ "nyn",			NULL,			"Nyankole" },
	{ "nyo",			NULL,			"Nyoro" },
	{ "nzi",			NULL,			"Nzima" },
	{ "oci",			"oc",			"Occitan (post 1500); Provenal" },
	{ "oji",			"oj",			"Ojibwa" },
	{ "ori",			"or",			"Oriya" },
	{ "orm",			"om",			"Oromo" },
	{ "osa",			NULL,			"Osage" },
	{ "oss",			"os",			"Ossetian; Ossetic" },
	{ "ota",			NULL,			"Turkish, Ottoman (1500-1928)" },
	{ "oto",			NULL,			"Otomian languages" },
	{ "paa",			NULL,			"Papuan (Other)" },
	{ "pag",			NULL,			"Pangasinan" },
	{ "pal",			NULL,			"Pahlavi" },
	{ "pam",			NULL,			"Pampanga" },
	{ "pan",			"pa",			"Panjabi; Punjabi" },
	{ "pap",			NULL,			"Papiamento" },
	{ "pau",			NULL,			"Palauan" },
	{ "peo",			NULL,			"Persian, Old (ca.600-400 B.C.)" },
	{ "per/fas",	"fa",			"Persian" }, //dual codes
	{ "phi",			NULL,			"Philippine (Other)" },
	{ "phn",			NULL,			"Phoenician" },
	{ "pli",			"pi",			"Pali" },
	{ "pol",			"pl",			"Polish" },
	{ "pon",			NULL,			"Pohnpeian" },
	{ "por",			"pt",			"Portuguese" },
	{ "pra",			NULL,			"Prakrit languages" },
	{ "pro",			NULL,			"Provenal, Old (to 1500)" },
	{ "pus",			"ps",			"Pushto" },
	//{ "qaa-qtz",	NULL,			"Reserved for local use" },
	{ "que",			"qu",			"Quechua" },
	{ "raj",			NULL,			"Rajasthani" },
	{ "rap",			NULL,			"Rapanui" },
	{ "rar",			NULL,			"Rarotongan" },
	{ "roa",			NULL,			"Romance (Other)" },
	{ "roh",			"rm",			"Raeto-Romance" },
	{ "rom",			NULL,			"Romany" },
	{ "rum/ron",	"ro",			"Romanian" }, //dual codes
	{ "run",			"rn",			"Rundi" },
	{ "rup",			NULL,			"Aromanian; Arumanian; Macedo-Romanian" },
	{ "rus",			"ru",			"Russian" },
	{ "sad",			NULL,			"Sandawe" },
	{ "sag",			"sg",			"Sango" },
	{ "sah",			NULL,			"Yakut" },
	{ "sai",			NULL,			"South American Indian (Other)" },
	{ "sal",			NULL,			"Salishan languages" },
	{ "sam",			NULL,			"Samaritan Aramaic" },
	{ "san",			"sa",			"Sanskrit" },
	{ "sas",			NULL,			"Sasak" },
	{ "sat",			NULL,			"Santali" },
	{ "scn",			NULL,			"Sicilian" },
	{ "sco",			NULL,			"Scots" },
	{ "scr/hrv",	"hr",			"Croatian" }, //dual codes
	{ "sel",			NULL,			"Selkup" },
	{ "sem",			NULL,			"Semitic (Other)" },
	{ "sga",			NULL,			"Irish, Old (to 900)" },
	{ "sgn",			NULL,			"Sign Languages" },
	{ "shn",			NULL,			"Shan" },
	{ "sid",			NULL,			"Sidamo" },
	{ "sin",			"si",			"Sinhala; Sinhalese" },
	{ "sio",			NULL,			"Siouan languages" },
	{ "sit",			NULL,			"Sino-Tibetan (Other)" },
	{ "sla",			NULL,			"Slavic (Other)" },
	{ "slo/slk",	"sk",			"Slovak" }, //dual codes
	{ "slv",			"sl",			"Slovenian" },
	{ "sma",			NULL,			"Southern Sami" },
	{ "sme",			"se",			"Northern Sami" },
	{ "smi",			NULL,			"Sami languages (Other)" },
	{ "smj",			NULL,			"Lule Sami" },
	{ "smn",			NULL,			"Inari Sami" },
	{ "smo",			"sm",			"Samoan" },
	{ "sms",			NULL,			"Skolt Sami" },
	{ "sna",			"sn",			"Shona" },
	{ "snd",			"sd",			"Sindhi" },
	{ "snk",			NULL,			"Soninke" },
	{ "sog",			NULL,			"Sogdian" },
	{ "som",			"so",			"Somali" },
	{ "son",			NULL,			"Songhai" },
	{ "sot",			"st",			"Sotho, Southern" },
	{ "spa",			"es",			"Spanish; Castilian" },
	{ "srd",			"sc",			"Sardinian" },
	{ "srn",			NULL,			"Sranan Togo" },
	{ "scc/srp",	"sr",			"Serbian" }, //dual codes
	{ "srr",			NULL,			"Serer" },
	{ "ssa",			NULL,			"Nilo-Saharan (Other)" },
	{ "ssw",			"ss",			"Swati" },
	{ "suk",			NULL,			"Sukuma" },
	{ "sun",			"su",			"Sundanese" },
	{ "sus",			NULL,			"Susu" },
	{ "sux",			NULL,			"Sumerian" },
	{ "swa",			"sw",			"Swahili" },
	{ "swe",			"sv",			"Swedish" },
	{ "syr",			NULL,			"Syriac" },
	{ "tah",			"ty",			"Tahitian" },
	{ "tai",			NULL,			"Tai (Other)" },
	{ "tam",			"ta",			"Tamil" },
	{ "tat",			"tt",			"Tatar" },
	{ "tel",			"te",			"Telugu" },
	{ "tem",			NULL,			"Timne" },
	{ "ter",			NULL,			"Tereno" },
	{ "tet",			NULL,			"Tetum" },
	{ "tgk",			"tg",			"Tajik" },
	{ "tgl",			"tl",			"Tagalog" },
	{ "tha",			"th",			"Thai" },
	{ "tib/bod",	"bo",			"Tibetan" }, //dual codes
	{ "tig",			NULL,			"Tigre" },
	{ "tir",			"ti",			"Tigrinya" },
	{ "tiv",			NULL,			"Tiv" },
	{ "tkl",			NULL,			"Tokelau" },
	{ "tlh",			NULL,			"Klingon; tlhIngan-Hol" },
	{ "tli",			NULL,			"Tlingit" },
	{ "tmh",			NULL,			"Tamashek" },
	{ "tog",			NULL,			"Tonga (Nyasa)" },
	{ "ton",			"to",			"Tonga (Tonga Islands)" },
	{ "tpi",			NULL,			"Tok Pisin" },
	{ "tsi",			NULL,			"Tsimshian" },
	{ "tsn",			"tn",			"Tswana" },
	{ "tso",			"ts",			"Tsonga" },
	{ "tuk",			"tk",			"Turkmen" },
	{ "tum",			NULL,			"Tumbuka" },
	{ "tup",			NULL,			"Tupi languages" },
	{ "tur",			"tr",			"Turkish" },
	{ "tut",			NULL,			"Altaic (Other)" },
	{ "tvl",			NULL,			"Tuvalu" },
	{ "twi",			"tw",			"Twi" },
	{ "tyv",			NULL,			"Tuvinian" },
	{ "udm",			NULL,			"Udmurt" },
	{ "uga",			NULL,			"Ugaritic" },
	{ "uig",			"ug",			"Uighur; Uyghur" },
	{ "ukr",			"uk",			"Ukrainian" },
	{ "umb",			NULL,			"Umbundu" },
	{ "und",			NULL,			"Undetermined" },
	{ "urd",			"ur",			"Urdu" },
	{ "uzb",			"uz",			"Uzbek" },
	{ "vai",			NULL,			"Vai" },
	{ "ven",			"ve",			"Venda" },
	{ "vie",			"vi",			"Vietnamese" },
	{ "vol",			"vo",			"VolapŸk" },
	{ "vot",			NULL,			"Votic" },
	{ "wak",			NULL,			"Wakashan languages" },
	{ "wal",			NULL,			"Walamo" },
	{ "war",			NULL,			"Waray" },
	{ "was",			NULL,			"Washo" },
	{ "wel/cym",	"cy",			"Welsh" }, // //dual codes
	{ "wen",			NULL,			"Sorbian languages" },
	{ "wln",			"wa",			"Walloon" },
	{ "wol",			"wo",			"Wolof" },
	{ "xal",			NULL,			"Kalmyk; Oirat" },
	{ "xho",			"xh",			"Xhosa" },
	{ "yao",			NULL,			"Yao" },
	{ "yap",			NULL,			"Yapese" },
	{ "yid",			"yi",			"Yiddish" },
	{ "yor",			"yo",			"Yoruba" },
	{ "ypk",			NULL,			"Yupik languages" },
	{ "zap",			NULL,			"Zapotec" },
	{ "zen",			NULL,			"Zenaga" },
	{ "zha",			"za",			"Zhuang; Chuang" },
	{ "chi/zho",	"zh",			"Chinese" }, //dual codes
	{ "znd",			NULL,			"Zande" },
	{ "zul",			"zu",			"Zulu" },
	{ "zun",			NULL,			"Zuni" },
	{ "zxx",			NULL,			"No linguistic content" }
};

m_ratings known_ratings[] = {
	{ "us-tv|TV-MA|600|",  "TV-MA" },
	{ "us-tv|TV-14|500|",  "TV-14" },
	{ "us-tv|TV-PG|400|",  "TV-PG" },
	{ "us-tv|TV-G|300|",   "TV-G" },
	{ "us-tv|TV-Y|200|",   "TV-Y" },
	{ "us-tv|TV-Y7|100|",  "TV-Y7" },
	//{ "us-tv||0|",         "not-applicable" }, //though its a valid flag & some files have this, AP won't be setting it.
	{ "mpaa|UNRATED|600|",  "Unrated" },
	{ "mpaa|NC-17|500|",    "NC-17" },
	{ "mpaa|R|400|",        "R" },
	{ "mpaa|PG-13|300|",    "PG-13" },
	{ "mpaa|PG|200|",       "PG" },
	{ "mpaa|G|100|",        "G" }
	//{ "mpaa||0|",         "not-applicable" } //see above
};

char* GenreIntToString(int genre) {
	char* return_string = NULL;
  if (genre > 0 &&  genre <= (int)(sizeof(ID3v1GenreList)/sizeof(*ID3v1GenreList))) {
		return_string = (char*)ID3v1GenreList[genre-1];
	}
	return return_string;
}

uint8_t StringGenreToInt(const char* genre_string) {
	uint8_t return_genre = 0;
	uint8_t total_genres = (uint8_t)(sizeof(ID3v1GenreList)/sizeof(*ID3v1GenreList));
	uint8_t genre_length = strlen(genre_string)+1;

	for(uint8_t i = 0; i < total_genres; i++) {
		if ( memcmp(genre_string, ID3v1GenreList[i], strlen(ID3v1GenreList[i])+1 > genre_length ? strlen(ID3v1GenreList[i])+1 : genre_length ) == 0) {
			return_genre = i+1; //the list starts at 0; the embedded genres start at 1
			//fprintf(stdout, "Genre %s is %i\n", ID3v1GenreList[i], return_genre);
			break;
		}
	}
	if ( return_genre > total_genres ) {
		return_genre = 0;
	}
	return return_genre;
}

void ListGenresValues() {
	uint8_t total_genres = (uint8_t)(sizeof(ID3v1GenreList)/sizeof(*ID3v1GenreList));
	fprintf(stdout, "\tAvailable standard genres - case sensitive.\n");

	for (uint8_t i = 0; i < total_genres; i++) {
		fprintf(stdout, "(%i.)  %s\n", i+1, ID3v1GenreList[i]);
	}
	return;
}

stiks* MatchStikString(const char* in_stik_string) {
	stiks* matching_stik = NULL;
	uint8_t total_known_stiks = (uint32_t)(sizeof(stikArray)/sizeof(*stikArray));
	uint8_t stik_str_length = strlen(in_stik_string) +1;
	
	for (uint8_t i = 0; i < total_known_stiks; i++) {
		if ( memcmp(in_stik_string, stikArray[i].stik_string, 
		                         strlen(stikArray[i].stik_string)+1 > stik_str_length ? strlen(stikArray[i].stik_string)+1 : stik_str_length ) == 0) {
			matching_stik = &stikArray[i];
			break;
		}
	}
	return matching_stik;
}

stiks* MatchStikNumber(uint8_t in_stik_num) {
	stiks* matching_stik = NULL;
	uint8_t total_known_stiks = (uint32_t)(sizeof(stikArray)/sizeof(*stikArray));
	
	for (uint8_t i = 0; i < total_known_stiks; i++) {
		if ( stikArray[i].stik_number == in_stik_num ) {
			matching_stik = &stikArray[i];
			break;
		}
	}
	return matching_stik;
}

void ListStikValues() {
	uint8_t total_known_stiks = (uint32_t)(sizeof(stikArray)/sizeof(*stikArray));
	fprintf(stdout, "\tAvailable stik settings - case sensitive  (number in parens shows the stik value).\n");

	for (uint8_t i = 0; i < total_known_stiks; i++) {
		fprintf(stdout, "(%u)  %s\n", stikArray[i].stik_number, stikArray[i].stik_string);
	}
	return;
}

sfIDs* MatchStoreFrontNumber(uint32_t storefrontnum) {
	sfIDs* matching_sfID = NULL;
	uint8_t total_known_sfs = (uint32_t)(sizeof(storefronts)/sizeof(*storefronts));
	
	for (uint8_t i = 0; i < total_known_sfs; i++) {
		if ( storefronts[i].storefront_number == storefrontnum ) {
			matching_sfID = &storefronts[i];
			break;
		}
	}
	return matching_sfID;
}

bool MatchLanguageCode(const char* in_code) {
	bool matching_lang = false;
	uint16_t total_known_langs = (uint16_t)(sizeof(known_languages)/sizeof(*known_languages));
	
	for (uint16_t i = 0; i < total_known_langs; i++) {
		if (memcmp(in_code, known_languages[i].iso639_2_code, 3) == 0) {
			matching_lang = true;
			break;
		}
		if (strlen(known_languages[i].iso639_2_code) > 3) {
			if (memcmp(in_code, known_languages[i].iso639_2_code+4, 3) == 0) {
				matching_lang = true;
				break;
			}
		}
	}

	return matching_lang;
}

void ListLanguageCodes() {
	uint16_t total_known_langs = (uint16_t)(sizeof(known_languages)/sizeof(*known_languages));
	fprintf(stdout, "\tAvailable language codes\nISO639-2 code  ... English name:\n");

	for (uint16_t i = 0; i < total_known_langs; i++) {
		fprintf(stdout, " %s  ... %s\n", known_languages[i].iso639_2_code, known_languages[i].language_in_english);
	}
	return;
}

void ListMediaRatings() {
	uint16_t total_known_ratings = (uint16_t)(sizeof(known_ratings)/sizeof(*known_ratings));
	fprintf(stdout, "\tAvailable ratings for the U.S. rating system:\n");

	for (uint16_t i = 0; i < total_known_ratings; i++) {
		fprintf(stdout, " %s\n", known_ratings[i].media_rating_cli_str);
	}
	return;
}

char* Expand_cli_mediastring(char* cli_rating) {
	char* media_rating = NULL;
	uint16_t total_known_ratings = (uint16_t)(sizeof(known_ratings)/sizeof(*known_ratings));
	uint8_t rating_len = strlen(cli_rating);
	
	for (uint16_t i = 0; i < total_known_ratings; i++) {
		if ( strncasecmp(known_ratings[i].media_rating_cli_str, cli_rating, rating_len+1) == 0 ) {
			media_rating = known_ratings[i].media_rating;
			break;
		}
	}
	return media_rating;
}

//ID32 for ID3 frame functions
char* ID3GenreIntToString(int genre) {
	char* return_string = NULL;
  if (genre >= 0 &&  genre <= 79) {
		return_string = (char*)ID3v1GenreList[genre];
	}
	return return_string;
}

uint8_t ID3StringGenreToInt(const char* genre_string) {
	uint8_t return_genre = 0xFF;
	uint8_t total_genres = 80;
	uint8_t genre_length = strlen(genre_string)+1;

	for(uint8_t i = 0; i < total_genres; i++) {
		if ( memcmp(genre_string, ID3v1GenreList[i], strlen(ID3v1GenreList[i])+1 > genre_length ? strlen(ID3v1GenreList[i])+1 : genre_length ) == 0) {
			return i;
		}
	}
	if ( return_genre > total_genres ) {
		return_genre = 0xFF;
	}
	return return_genre;
}
