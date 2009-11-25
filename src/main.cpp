//==================================================================//
/*
    AtomicParsley - main.cpp

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
		
    * Mike Brancato - Debian patches & build support
		* Brian Story - porting getopt & native Win32 patches
                                                                   */
//==================================================================//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <wchar.h>

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#if defined HAVE_GETOPT_H
#include <getopt.h>
#else
#include "./extras/getopt.h"
#endif

#include "AP_commons.h"
#include "AtomicParsley.h"
#include "AP_AtomExtracts.h"
#include "AP_iconv.h"                 /* for xmlInitEndianDetection used in endian utf16 conversion */
#include "AP_arrays.h"
#include "APar_uuid.h"
#include "AP_ID3v2_tags.h"
#include "AP_MetadataListings.h"

// define one-letter cli options for
#define OPT_HELP                 'h'
#define OPT_TEST		             'T'
#define OPT_ShowTextData		     't'
#define OPT_ExtractPix           'E'
#define OPT_ExtractPixToPath		 'e'
#define Meta_artist              'a'
#define Meta_songtitle           's'
#define Meta_album               'b'
#define Meta_tracknum            'k'
#define Meta_disknum             'd'
#define Meta_genre               'g'
#define Meta_comment             'c'
#define Meta_year                'y'
#define Meta_lyrics              'l'
#define Meta_composer            'w'
#define Meta_copyright           'x'
#define Meta_grouping            'G'
#define Meta_album_artist        'A'
#define Meta_compilation         'C'
#define Meta_BPM                 'B'
#define Meta_artwork             'r'
#define Meta_advisory            'V'
#define Meta_stik                'S'
#define Meta_description         'p'
#define Meta_TV_Network          'n'
#define Meta_TV_ShowName         'H'
#define Meta_TV_EpisodeNumber    'N'
#define Meta_TV_SeasonNumber     'U'
#define Meta_TV_Episode          'I'
#define Meta_podcastFlag         'f'
#define Meta_category            'q'
#define Meta_keyword             'K'
#define Meta_podcast_URL         'L'
#define Meta_podcast_GUID        'J'
#define Meta_PurchaseDate        'D'
#define Meta_EncodingTool        0xB7
#define Meta_PlayGapless         0xBA
#define Meta_SortOrder           0xBF

#define Meta_ReverseDNS_Form     'M'
#define Meta_rDNS_rating         0xBB

#define Meta_StandardDate        'Z'
#define Meta_URL                 'u'
#define Meta_Information         'i'
#define Meta_uuid                'z'
#define Opt_Extract_all_uuids    0xB8
#define Opt_Extract_a_uuid       0xB9
#define Opt_Ipod_AVC_uuid        0xBE

#define Metadata_Purge           'P'
#define UserData_Purge           'X'
#define foobar_purge             '.'
#define Meta_dump                'Q'
#define Manual_atom_removal      'R'
#define Opt_FreeFree             'F'
#define OPT_OutputFile           'o'
#define OPT_NoOptimize           0xBD

#define OPT_OverWrite            'W'

#define ISO_Copyright            0xAA

#define _3GP_Title               0xAB
#define _3GP_Author              0xAC
#define _3GP_Performer           0xAD
#define _3GP_Genre               0xAE
#define _3GP_Description         0xAF
#define _3GP_Copyright           0xB0
#define _3GP_Album               0xB1
#define _3GP_Year                0xB2
#define _3GP_Rating              0xB3
#define _3GP_Classification      0xB4
#define _3GP_Keyword             0xB5
#define _3GP_Location            0xB6

#define Meta_ID3v2Tag            0xBC

char *output_file;

int total_args;

static void kill_signal ( int sig );

static void kill_signal (int sig) {
    exit(0);
}

//less than 80 (max 78) char wide, giving a general (concise) overview
static char* shortHelp_text =
"\n"
"AtomicParlsey sets metadata into MPEG-4 files & derivatives supporting 3 tag\n"
" schemes: iTunes-style, 3GPP assets & ISO defined copyright notifications.\n"
"\n"
"AtomicParlsey quick help for setting iTunes-style metadata into MPEG-4 files.\n"
"\n"
"General usage examples:\n"
"  AtomicParsley /path/to.mp4 -T 1\n"
"  AtomicParsley /path/to.mp4 -t +\n"
"  AtomicParsley /path/to.mp4 --artist \"Me\" --artwork /path/to/art.jpg\n"
"  Atomicparsley /path/to.mp4 --albumArtist \"You\" --podcastFlag true\n"
"  Atomicparsley /path/to.mp4 --stik \"TV Show\" --advisory explicit\n"
"\n"
"Getting information about the file & tags:\n"
"  -T  --test        Test file for mpeg4-ishness & print atom tree\n"
"  -t  --textdata    Prints tags embedded within the file\n"
"  -E  --extractPix  Extracts pix to the same folder as the mpeg-4 file\n"
"\n"
"Setting iTunes-style metadata tags\n"
"  --artist       (string)     Set the artist tag\n"
"  --title        (string)     Set the title tag\n"
"  --album        (string)     Set the album tag\n"
"  --genre        (string)     Genre tag (see --longhelp for more info)\n"
"  --tracknum     (num)[/tot]  Track number (or track number/total tracks)\n"
"  --disk         (num)[/tot]  Disk number (or disk number/total disks)\n"
"  --comment      (string)     Set the comment tag\n"
"  --year         (num|UTC)    Year tag (see --longhelp for \"Release Date\")\n"
"  --lyrics       (string)     Set lyrics (not subject to 256 byte limit)\n"
"  --composer     (string)     Set the composer tag\n"
"  --copyright    (string)     Set the copyright tag\n"
"  --grouping     (string)     Set the grouping tag\n"
"  --artwork      (/path)      Set a piece of artwork (jpeg or png only)\n"
"  --bpm          (number)     Set the tempo/bpm\n"
"  --albumArtist  (string)     Set the album artist tag\n"
"  --compilation  (boolean)    Set the compilation flag (true or false)\n"
"  --advisory     (string*)    Content advisory (*values: 'clean', 'explicit')\n"
"  --stik         (string*)    Sets the iTunes \"stik\" atom (see --longhelp)\n"
"  --description  (string)     Set the description tag\n"
"  --TVNetwork    (string)     Set the TV Network name\n"
"  --TVShowName   (string)     Set the TV Show name\n"
"  --TVEpisode    (string)     Set the TV episode/production code\n"
"  --TVSeasonNum  (number)     Set the TV Season number\n"
"  --TVEpisodeNum (number)     Set the TV Episode number\n"
"  --podcastFlag  (boolean)    Set the podcast flag (true or false)\n"
"  --category     (string)     Sets the podcast category\n"
"  --keyword      (string)     Sets the podcast keyword\n"
"  --podcastURL   (URL)        Set the podcast feed URL\n"
"  --podcastGUID  (URL)        Set the episode's URL tag\n"
"  --purchaseDate (UTC)        Set time of purchase\n"
"  --encodingTool (string)     Set the name of the encoder\n"
"  --gapless      (boolean)    Set the gapless playback flag\n"
"  --contentRating (string*)   Set tv/mpaa rating (see -rDNS-help)\n"
"\n"
"Deleting tags\n"
"  Set the value to \"\":        --artist \"\" --stik \"\" --bpm \"\"\n"
"  To delete (all) artwork:    --artwork REMOVE_ALL\n"
"  manually removal:           --manualAtomRemove \"moov.udta.meta.ilst.ATOM\"\n"
"\n"
"More detailed iTunes help is available with AtomicParsley --longhelp\n"
"Setting reverse DNS forms for iTunes files: see --reverseDNS-help\n"
"Setting 3gp assets into 3GPP & derivative files: see --3gp-help\n"
"Setting copyright notices for all files: see --ISO-help\n"
"For file-level options & padding info: see --file-help\n"
"Setting custom private tag extensions: see --uuid-help\n"
"Setting ID3 tags onto mpeg-4 files: see --ID3-help\n"
"----------------------------------------------------------------------"
;

//an expansive, verbose, unconstrained (about 112 char wide) detailing of options
static char* longHelp_text =
"AtomicParsley help page for setting iTunes-style metadata into MPEG-4 files. \n"
"              (3gp help available with AtomicParsley --3gp-help)\n"
"          (ISO copyright help available with AtomicParsley --ISO-help)\n"
"      (reverse DNS form help available with AtomicParsley --reverseDNS-help)\n"
"Usage: AtomicParsley [mp4FILE]... [OPTION]... [ARGUMENT]... [ [OPTION2]...[ARGUMENT2]...] \n"
"\n"
"example: AtomicParsley /path/to.mp4 -e ~/Desktop/pix\n"
"example: AtomicParsley /path/to.mp4 --podcastURL \"http://www.url.net\" --tracknum 45/356\n"
"example: AtomicParsley /path/to.mp4 --copyright \"\342\204\227 \302\251 2006\"\n"
"example: AtomicParsley /path/to.mp4 --year \"2006-07-27T14:00:43Z\" --purchaseDate timestamp\n"
"example: AtomicParsley /path/to.mp4 --sortOrder artist \"Mighty Dub Cats, The\n"
"------------------------------------------------------------------------------------------------\n"
"  Extract any pictures in user data \"covr\" atoms to separate files. \n"
"  --extractPix       ,  -E                     Extract to same folder (basename derived from file).\n"
"  --extractPixToPath ,  -e  (/path/basename)   Extract to specific path (numbers added to basename).\n"
"                                                 example: --e ~/Desktop/SomeText\n"
"                                                 gives: SomeText_artwork_1.jpg  SomeText_artwork_2.png\n"
"                                               Note: extension comes from embedded image file format\n"
"------------------------------------------------------------------------------------------------\n"
" Tag setting options:\n"
"\n"
"  --artist           ,  -a   (str)    Set the artist tag: \"moov.udta.meta.ilst.\302©ART.data\"\n"
"  --title            ,  -s   (str)    Set the title tag: \"moov.udta.meta.ilst.\302©nam.data\"\n"
"  --album            ,  -b   (str)    Set the album tag: \"moov.udta.meta.ilst.\302©alb.data\"\n"
"  --genre            ,  -g   (str)    Set the genre tag: \"\302©gen\" (custom) or \"gnre\" (standard).\n"
"                                          see the standard list with \"AtomicParsley --genre-list\"\n"
"  --tracknum         ,  -k   (num)[/tot]  Set the track number (or track number & total tracks).\n"
"  --disk             ,  -d   (num)[/tot]  Set the disk number (or disk number & total disks).\n"
"  --comment          ,  -c   (str)    Set the comment tag: \"moov.udta.meta.ilst.\302©cmt.data\"\n"
"  --year             ,  -y   (num|UTC)    Set the year tag: \"moov.udta.meta.ilst.\302©day.data\"\n"
"                                          set with UTC \"2006-09-11T09:00:00Z\" for Release Date\n"
"  --lyrics           ,  -l   (str)    Set the lyrics tag: \"moov.udta.meta.ilst.\302©lyr.data\"\n"
"  --composer         ,  -w   (str)    Set the composer tag: \"moov.udta.meta.ilst.\302©wrt.data\"\n"
"  --copyright        ,  -x   (str)    Set the copyright tag: \"moov.udta.meta.ilst.cprt.data\"\n"
"  --grouping         ,  -G   (str)    Set the grouping tag: \"moov.udta.meta.ilst.\302©grp.data\"\n"
"  --artwork          ,  -A   (/path)  Set a piece of artwork (jpeg or png) on \"covr.data\"\n"
"                                          Note: multiple pieces are allowed with more --artwork args\n"
"  --bpm              ,  -B   (num)    Set the tempo/bpm tag: \"moov.udta.meta.ilst.tmpo.data\"\n"
"  --albumArtist      ,  -A   (str)    Set the album artist tag: \"moov.udta.meta.ilst.aART.data\"\n"
"  --compilation      ,  -C   (bool)   Sets the \"cpil\" atom (true or false to delete the atom)\n"
"  --advisory         ,  -y   (1of3)   Sets the iTunes lyrics advisory ('remove', 'clean', 'explicit') \n"
"  --stik             ,  -S   (1of7)   Sets the iTunes \"stik\" atom (--stik \"remove\" to delete) \n"
"                                           \"Movie\", \"Normal\", \"TV Show\" .... others: \n"
"                                           see the full list with \"AtomicParsley --stik-list\"\n"
"                                           or set in an integer value with --stik value=(num)\n"
"                                      Note: --stik Audiobook will change file extension to '.m4b'\n"
"  --description      ,  -p   (str)    Sets the description on the \"desc\" atom\n"
"  --TVNetwork        ,  -n   (str)    Sets the TV Network name on the \"tvnn\" atom\n"
"  --TVShowName       ,  -H   (str)    Sets the TV Show name on the \"tvsh\" atom\n"
"  --TVEpisode        ,  -I   (str)    Sets the TV Episode on \"tven\":\"209\", but its a string: \"209 Part 1\"\n"
"  --TVSeasonNum      ,  -U   (num)    Sets the TV Season number on the \"tvsn\" atom\n"
"  --TVEpisodeNum     ,  -N   (num)    Sets the TV Episode number on the \"tves\" atom\n"

"  --podcastFlag      ,  -f   (bool)   Sets the podcast flag (values are \"true\" or \"false\")\n"
"  --category         ,  -q   (str)    Sets the podcast category; typically a duplicate of its genre\n"
"  --keyword          ,  -K   (str)    Sets the podcast keyword; invisible to MacOSX Spotlight\n"
"  --podcastURL       ,  -L   (URL)    Set the podcast feed URL on the \"purl\" atom\n"
"  --podcastGUID      ,  -J   (URL)    Set the episode's URL tag on the \"egid\" atom\n"
"  --purchaseDate     ,  -D   (UTC)    Set Universal Coordinated Time of purchase on a \"purd\" atom\n"
"                                       (use \"timestamp\" to set UTC to now; can be akin to id3v2 TDTG tag)\n"
"  --encodingTool     ,       (str)    Set the name of the encoder on the \"\302©too\" atom\n"
"  --gapless          ,       (bool)   Sets the gapless playback flag for a track in a gapless album\n"

"  --sortOrder    (type)      (str)    Sets the sort order string for that type of tag.\n"
"                                       (available types are: \"name\", \"artist\", \"albumartist\",\n"
"                                        \"album\", \"composer\", \"show\")\n"
"\n"
"NOTE: Except for artwork, only 1 of each tag is allowed; artwork allows multiple pieces.\n"
"NOTE: Tags that carry text(str) have a limit of 255 utf8 characters; lyrics have no limit.\n"
"------------------------------------------------------------------------------------------------\n"
" To delete a single atom, set the tag to null (except artwork):\n"
"  --artist \"\" --lyrics \"\"\n"
"  --artwork REMOVE_ALL \n"
"  --metaEnema        ,  -P            Douches away every atom under \"moov.udta.meta.ilst\" \n"
"  --foobar2000Enema  ,  -2            Eliminates foobar2000's non-compliant so-out-o-spec tagging scheme\n"
"  --manualAtomRemove \"some.atom.path\" where some.atom.path can be:\n"
"      keys to using manualAtomRemove:\n"
"         ilst.ATOM.data or ilst.ATOM target an iTunes-style metadata tag\n"
"         ATOM:lang=foo               target an atom with this language setting; like 3gp assets\n"
"         ATOM.----.name:[foo]        target a reverseDNS metadata tag; like iTunNORM\n"
"                                     Note: these atoms show up with 'AP -t' as: Atom \"----\" [foo]\n"
"                                         'foo' is actually carried on the 'name' atom\n"
"         ATOM[x]                     target an atom with an index other than 1; like trak[2]\n"
"         ATOM.uuid=hex-hex-hex-hex   targt a uuid atom with the uuid of hex string representation\n"
"    examples:\n"
"        moov.udta.meta.ilst.----.name:[iTunNORM]      moov.trak[3].cprt:lang=urd\n"
"        moov.trak[2].uuid=55534d54-21d2-4fce-bb88-695cfac9c740\n"
"------------------------------------------------------------------------------------------------\n"

#if defined (DARWIN_PLATFORM)
"                   Environmental Variables (affecting picture placement)\n"
"\n"
"  set PIC_OPTIONS in your shell to set these flags; preferences are separated by colons (:)\n"
"\n"
" MaxDimensions=num (default: 0; unlimited); sets maximum pixel dimensions\n"
" DPI=num           (default: 72); sets dpi\n"
" MaxKBytes=num     (default: 0; unlimited);  maximum kilobytes for file (jpeg only)\n"
" AddBothPix=bool   (default: false); add original & converted pic (for archival purposes)\n"
" AllPixJPEG | AllPixPNG =bool (default: false); force conversion to a specific picture format\n"
" SquareUp          (include to square images to largest dimension, allows an [ugly] 160x1200->1200x1200)\n"
" removeTempPix     (include to delete temp pic files created when resizing images after tagging)\n"
" ForceHeight=num   (must also specify width, below) force image pixel height\n"
" ForceWidth=num    (must also specify height, above) force image pixel width\n"
"\n"
" Examples: (bash-style)\n"
" export PIC_OPTIONS=\"MaxDimensions=400:DPI=72:MaxKBytes=100:AddBothPix=true:AllPixJPEG=true\"\n"
" export PIC_OPTIONS=\"SquareUp:removeTempPix\"\n"
" export PIC_OPTIONS=\"ForceHeight=999:ForceWidth=333:removeTempPix\"\n"
"------------------------------------------------------------------------------------------------\n"
#endif
;

static char* fileLevelHelp_text =
"AtomicParsley help page for general & file level options.\n"
#if defined (_MSC_VER)
"  Note: you can change the input/output behavior to raw 8-bit utf8 if the program name\n"
"        is appended with \"-utf8\". AtomicParsley-utf8.exe will have problems with files/\n"
"        folders with unicode characters in given paths.\n"
"\n"
#endif
"------------------------------------------------------------------------------------------------\n"
" Atom reading services:\n"
"\n"
"  --test             ,  -T           Tests file to see if its a valid MPEG-4 file.\n"
"                                     Prints out the hierarchical atom tree.\n"
"                        -T 1         Supplemental track level info with \"-T 1\"\n"
"                        -T +dates    Track level with creation/modified dates\n"
"\n"
"  --textdata         ,  -t      print user data text metadata relevant to brand (inc. # of any pics).\n"
"                        -t +    show supplemental info like free space, available padding, user data\n"
"                                length & media data length\n"
"                        -t 1    show all textual metadata (disregards brands, shows track copyright)\n"
"\n"
"  --brands                      show the major & minor brands for the file & available tagging schemes\n"
"\n"
"------------------------------------------------------------------------------------------------\n"
" File services:\n"
"\n"
"  --freefree [num]   ,                Remove \"free\" atoms which only act as filler in the file\n"
"                                      ?(num)? - optional integer argument to delete 'free's to desired level\n"
"\n"
"                                      NOTE 1: levels begin at level 1 aka file level.\n"
"                                      NOTE 2: Level 0 (which doesn't exist) deletes level 1 atoms that pre-\n"
"                                              cede 'moov' & don't serve as padding. Typically, such atoms\n"
"                                              are created by libmp4ff or libmp4v2 as a byproduct of tagging.\n"
"                                      NOTE 3: When padding falls below MIN_PAD (typically zero), a default\n"
"                                              amount of padding (typically 2048 bytes) will be added. To\n"
"                                              achieve absolutely 0 bytes 'free' space with --freefree, set\n"
"                                              DEFAULT_PAD to 0 via the AP_PADDING mechanism (see below).\n"
"\n"
"  --preventOptimizing                 Prevents reorganizing the file to have file metadata before media data.\n"
"                                      iTunes/Quicktime have so far *always* placed metadata first; many 3rd\n"
"                                      party utilities do not (preventing streaming to the web, AirTunes, iTV).\n"
"                                      Used in conjunction with --overWrite, files with metadata at the end\n"
"                                      (most ffmpeg produced files) can have their tags rapidly updated without\n"
"                                      requiring a full rewrite. Note: this does not un-optimize a file.\n"
"                                      Note: this option will be canceled out if used with the --freefree option\n"
"\n"
"  --metaDump                          Dumps out 'moov.udta' metadata out to a new file next to original\n"
"                                          (for diagnostic purposes, please remove artwork before sending)\n"
"  --output           ,  -o   (/path)  Specify the filename of tempfile (voids overWrite)\n"
"  --overWrite        ,  -W            Writes to temp file; deletes original, renames temp to original\n"
"                                      If possible, padding will be used to update without a full rewrite.\n"
"\n"
"  --DeepScan                          Parse areas of the file that are normally skipped (must be the 3rd arg)\n"
"  --iPod-uuid                (num)    Place the ipod-required uuid for higher resolution avc video files\n"
"                                      Currently, the only number used is 1200 - the maximum number of macro-\n"
"                                      blocks allowed by the higher resolution iPod setting.\n"
"                                      NOTE: this requires the \"--DeepScan\" option as the 3rd cli argument\n"
"                                      NOTE2: only works on the first avc video track, not all avc tracks\n"
"\n"
"Examples: \n"
"  --freefree 0         (deletes all top-level non-padding atoms preceding 'mooov') \n"
"  --freefree 1         (deletes all non-padding atoms at the top most level) \n"
"  --output ~/Desktop/newfile.mp4\n"
"  AP /path/to/file.m4v --DeepScan --iPod-uuid 1200\n"

"------------------------------------------------------------------------------------------------\n"
" Padding & 'free' atoms:\n"
"\n"
"  A special type of atom called a 'free' atom is used for padding (all 'free' atoms contain NULL space).\n"
"  When changes need to occur, these 'free' atom are used. They grows or shink, but the relative locations\n"
"  of certain other atoms (stco/mdat) remain the same. If there is no 'free' space, a full rewrite will occur.\n"
"  The locations of 'free' atom(s) that AP can use as padding must be follow 'moov.udta' & come before 'mdat'.\n"
"  A 'free' preceding 'moov' or following 'mdat' won't be used as padding for example. \n"
"\n"
"  Set the shell variable AP_PADDING with these values, separated by colons to alter padding behavior:\n"
"\n"
"  DEFAULT_PADDING=  -  the amount of padding added if the minimum padding is non-existant in the file\n"
"                       default = 2048\n"
"  MIN_PAD=          -  the minimum padding present before more padding will be added\n"
"                       default = 0\n"
"  MAX_PAD=          -  the maximum allowable padding; excess padding will be eliminated\n"
"                       default = 5000\n"
"\n"
"  If you use --freefree to eliminate 'free' atoms from the file, the DEFAULT_PADDING amount will still be\n"
"  added to any newly written files. Set DEFAULT_PADDING=0 to prevent any 'free' padding added at rewrite.\n"
"  You can set MIN_PAD to be assured that at least that amount of padding will be present - similarly,\n"
"  MAX_PAD limits any excessive amount of padding. All 3 options will in all likelyhood produce a full\n"
"  rewrite of the original file. Another case where a full rewrite will occur is when the original file\n"
"  is not optimized and has 'mdat' preceding 'moov'.\n"
"\n"
#if defined (_MSC_VER)
"Examples:\n"
"   c:> SET AP_PADDING=\"DEFAULT_PAD=0\"      or    c:> SET AP_PADDING=\"DEFAULT_PAD=3128\"\n"
"   c:> SET AP_PADDING=\"DEFAULT_PAD=5128:MIN_PAD=200:MAX_PAD=6049\"\n"
#else
"Examples (bash style):\n"
"   $ export AP_PADDING=\"DEFAULT_PAD=0\"      or    $ export AP_PADDING=\"DEFAULT_PAD=3128\"\n"
"   $ export AP_PADDING=\"DEFAULT_PAD=5128:MIN_PAD=200:MAX_PAD=6049\"\n"
#endif
"\n"
"Note: while AtomicParsley is still in the beta stage, the original file will always remain untouched - \n"
"      unless given the --overWrite flag when if possible, utilizing available padding to update tags\n"
"      will be tried (falling back to a full rewrite if changes are greater than the found padding).\n"
"----------------------------------------------------------------------------------------------------\n"
" iTunes 7 & Gapless playback:\n"
"\n"
" iTunes 7 adds NULL space at the ends of files (filled with zeroes). It is possble this is how iTunes\n"
" implements gapless playback - perhaps not. In any event, with AtomicParsley you can choose to preserve\n"
" that NULL space, or you can eliminate its presence (typically around 2,000 bytes). The default behavior\n"
" is to preserve it - if it is present at all. You can choose to eliminate it by setting the environ-\n"
" mental preference for AP_PADDING to have DEFAULT_PAD=0\n"
"\n"
#if defined (_MSC_VER)
"Example:\n"
"   c:> SET AP_PADDING=\"DEFAULT_PAD=0\"\n"
#else
"Example (bash style):\n"
"   $ export AP_PADDING=\"DEFAULT_PAD=0\"\n"
#endif
"----------------------------------------------------------------------------------------------------\n"
;

//detailed options for 3gp branded files
static char* _3gpHelp_text =
"AtomicParsley 3gp help page for setting 3GPP-style metadata.\n"
"----------------------------------------------------------------------------------------------------\n"
"  3GPP text tags can be encoded in either UTF-8 (default input encoding) or UTF-16 (converted from UTF-8)\n"
"  Many 3GPP text tags can be set for a desired language by a 3-letter-lowercase code (default is \"eng\").\n"
"  For tags that support the language attribute (all except year), more than one tag of the same name\n"
"  (3 titles for example) differing in the language code is supported.\n"
"\n"
"  iTunes-style metadata is not supported by the 3GPP TS 26.244 version 6.4.0 Release 6 specification.\n"
"  3GPP asset tags can be set at movie level or track level & are set in a different hierarchy: moov.udta \n"
"  if at movie level (versus iTunes moov.udta.meta.ilst). Other 3rd party utilities may allow setting\n"
"  iTunes-style metadata in 3gp files. When a 3gp file is detected (file extension doesn't matter), only\n"
"  3gp spec-compliant metadata will be read & written.\n"
"\n"
"  Note1: there are a number of different 'brands' that 3GPP files come marked as. Some will not be \n"
"         supported by AtomicParsley due simply to them being unknown and untested. You can compile your\n"
"         own AtomicParsley to evaluate it by adding the hex code into the source of APar_IdentifyBrand.\n"
"\n"
"  Note2: There are slight accuracy discrepancies in location's fixed point decimals set and retrieved.\n"
"\n"
"  Note3: QuickTime Player can see a limited subset of these tags, but only in 1 language & there seems to\n"
"         be an issue with not all unicode text displaying properly. This is an issue withing QuickTime -\n"
"         the exact same text (in utf8) displays properly in an MPEG-4 file. Some languages can also display\n"
"         more glyphs than others.\n"
"\n"
"----------------------------------------------------------------------------------------------------\n"
" Tag setting options (default user data area is movie level; default lang is 'eng'; default encoding is UTF8):\n"
"     required arguments are in (parentheses); optional arguments are in [brackets]\n"
"\n"
"  --3gp-title           (str)  [lang=3str]  [UTF16]  [area]  .........  Set a 3gp media title tag\n"
"  --3gp-author          (str)  [lang=3str]  [UTF16]  [area]  .........  Set a 3gp author of the media tag\n"
"  --3gp-performer       (str)  [lang=3str]  [UTF16]  [area]  .........  Set a 3gp performer or artist tag\n"
"  --3gp-genre           (str)  [lang=3str]  [UTF16]  [area]  .........  Set a 3gp genre asset tag\n"
"  --3gp-description     (str)  [lang=3str]  [UTF16]  [area]  .........  Set a 3gp description or caption tag\n"
"  --3gp-copyright       (str)  [lang=3str]  [UTF16]  [area]  .........  Set a 3gp copyright notice tag*\n"
"\n"
"  --3gp-album           (str)  [lang=3str]  [UTF16]  [trknum=int] [area]  Set a 3gp album tag (& opt. tracknum)\n"
"  --3gp-year            (int)  [area]  ...........................  Set a 3gp recording year tag (4 digit only)\n"
"\n"
"  --3gp-rating          (str)  [entity=4str]  [criteria=4str]  [lang=3str]  [UTF16]  [area]   Rating tag\n"
"  --3gp-classification  (str)  [entity=4str]  [index=int]      [lang=3str]  [UTF16]  [area]   Classification\n"
"\n"
"  --3gp-keyword         (str)   [lang=3str]  [UTF16]  [area]   Format of str: 'keywords=word1,word2,word3,word4'\n"
"\n"
"  --3gp-location        (str)   [lang=3str]  [UTF16]  [area]   Set a 3gp location tag (default: Central Park)\n"
"                                 [longitude=fxd.pt]  [latitude=fxd.pt]  [altitude=fxd.pt]\n"
"                                 [role=str]  [body=str]  [notes=str]\n"
"                                 fxd.pt values are decimal coordinates (55.01209, 179.25W, 63)\n"
"                                 'role=' values: 'shooting location', 'real location', 'fictional location'\n"
"                                         a negative value in coordinates will be seen as a cli flag\n"
"                                         append 'S', 'W' or 'B': lat=55S, long=90.23W, alt=90.25B\n"
"\n"
" [area] can be \"movie\", \"track\" or \"track=num\" where 'num' is the number of the track. If not specificied,\n"
" assets will be placed at movie level. The \"track\" option sets the asset across all available tracks\n"
"\n"
"Note1: '4str' = a 4 letter string like \"PG13\"; 3str is a 3 letter string like \"eng\"; int is an integer\n"
"Note2: List all languages for '3str' with \"AtomicParsley --language-list (unknown languages become \"und\")\n"
"*Note3: The 3gp copyright asset can potentially be altered by using the --ISO-copyright setting.\n"

"----------------------------------------------------------------------------------------------------\n"
"Usage: AtomicParsley [3gpFILE] --option [argument] [optional_arguments]  [ --option2 [argument2]...] \n"
"\n"
"example: AtomicParsley /path/to.3gp -t \n"
"example: AtomicParsley /path/to.3gp -T 1 \n"
"example: Atomicparsley /path/to.3gp --3gp-performer \"Enjoy Yourself\" lang=pol UTF16\n"
"example: Atomicparsley /path/to.3gp --3gp-year 2006 --3gp-album \"White Label\" track=8 lang=fra\n"
"example: Atomicparsley /path/to.3gp --3gp-album \"Cow Cod Soup For Everyone\" track=10 lang=car\n"
"\n"
"example: Atomicparsley /path/to.3gp --3gp-classification \"Poor Sport\" entity=\"PTA \" index=12 UTF16\n"
"example: Atomicparsley /path/to.3gp --3gp-keyword keywords=\"foo1,foo2,foo 3\" UTF16 --3gp-keyword \"\"\n"
"example: Atomicparsley /path/to.3gp --3gp-location 'Bethesda Terrace' latitude=40.77 longitude=73.98W \n"
"                                                    altitude=4.3B role='real' body=Earth notes='Underground'\n"
"\n"
"example: Atomicparsley /path/to.3gp --3gp-title \"I see London.\" --3gp-title \"Veo Madrid.\" lang=spa \n"
"                                    --3gp-title \"Widze Warsawa.\" lang=pol\n"
"\n"
;

static char* ISOHelp_text =
"AtomicParsley help page for setting ISO copyright notices at movie & track level.\n"
"----------------------------------------------------------------------------------------------------\n"
"  The ISO specification allows for setting copyright in a number of places. This copyright atom is\n"
"  independant of the iTunes-style --copyright tag that can be set. This ISO tag is identical to the\n"
"  3GP-style copyright. In fact, using --ISO-copyright can potentially overwrite the 3gp copyright\n"
"  asset if set at movie level & given the same language to set the copyright on. This copyright\n"
"  notice is the only metadata tag defined by the reference ISO 14496-12 specification.\n"
"\n"
"  ISO copyright notices can be set at movie level, track level for a single track, or for all tracks.\n"
"  Multiple copyright notices are allowed, but they must differ in the language setting. To see avail-\n"
"  able languages use \"AtomicParsley --language-list\". Notices can be set in utf8 or utf16.\n"
"\n"
"  --ISO-copyright  (str)  [movie|track|track=#]  [lang=3str]  [UTF16]   Set a copyright notice\n"
"                                                           # in 'track=#' denotes the target track\n"
"                                                           3str is the 3 letter ISO-639-2 language.\n"
"                                                           Brackets [] show optional parameters.\n"
"                                                           Defaults are: movie level, 'eng' in utf8.\n"
"\n"
"example: AtomicParsley /path/file.mp4 -t 1      Note: the only way to see all contents is with -t 1 \n"
"example: AtomicParsley /path/file.mp4 --ISO-copyright \"Sample\"\n"
"example: AtomicParsley /path/file.mp4 --ISO-copyright \"Sample\" movie\n"
"example: AtomicParsley /path/file.mp4 --ISO-copyright \"Sample\" track=2 lang=urd\n"
"example: AtomicParsley /path/file.mp4 --ISO-copyright \"Sample\" track UTF16\n"
"example: AP --ISO-copyright \"Example\" track --ISO-copyright \"Por Exemplo\" track=2 lang=spa UTF16\n"
"\n"
"Note: to remove the copyright, set the string to \"\" - the track and language must match the target.\n"
"example: --ISO-copyright \"\" track --ISO-copyright \"\" track=2 lang=spa\n"
"\n"
"Note: (foo) denotes required arguments; [foo] denotes optional parameters & may have defaults.\n"
;

static char* uuidHelp_text =
"AtomicParsley help page for setting uuid user extension metadata tags.\n"
"----------------------------------------------------------------------------------------------------\n"
" Setting a user-defined 'uuid' private extention tags will appear in \"moov.udta.meta\"). These will\n"
" only be read by AtomicParsley & can be set irrespective of file branding. The form of uuid that AP\n"
" is a v5 uuid generated from a sha1 hash of an atom name in an 'AtomicParsley.sf.net' namespace.\n"
"\n"
" The uuid form is in some Sony & Compressor files, but of version 4 (random/pseudo-random). An example\n"
" uuid of 'cprt' in the 'AtomicParsley.sf.net' namespace is: \"4bd39a57-e2c8-5655-a4fb-7a19620ef151\".\n"
" 'cprt' in the same namespace will always create that uuid; uuid atoms will only print out if the\n"
" uuid generated is the same as discovered. Sony uuids don't for example show up with AP -t.\n"
"\n"
"  --information      ,  -i   (str)    Set an information tag on uuid atom name\"©inf\"\n"
"  --url              ,  -u   (URL)    Set a URL tag on uuid atom name \"\302©url\"\n"
"  --tagtime          ,      timestamp Set the Coordinated Univeral Time of tagging on \"tdtg\"\n"
"\n"
"  Define & set an arbitrary atom with a text data or embed a file:\n"
"  --meta-uuid        There are two forms: 1 for text & 1 for file operations\n"
"         setting text form:\n"
"         --meta-uuid   (atom) \"text\" (str)         \"atom\" = 4 character atom name of your choice\n"
"                                                     str is whatever text you want to set\n"
"         file embedding form:\n"
"         --meta-uuid   (atom) \"file\" (/path) [description=\"foo\"] [mime-type=\"foo/moof\"]\n"
"                                                     \"atom\" = 4 character atom name of your choice\n"
"                                                     /path = path to the file that will be embedded*\n"
"                                                     description = optional description of the file\n"
"                                                               default is \"[none]\"\n"
"                                                     mime-type = optional mime type for the file\n"
"                                                               default is \"none\"\n"
"                                                               Note: no auto-disocevery of mime type\n"
"                                                                     if you know/want it: supply it.\n"
"                                               *Note: a file extension (/path/file.ext) is required\n"
"\n"
"Note: (foo) denotes required arguments; [foo] denotes optional arguments & may have defaults.\n"
"\n"
"Examples: \n"
"  --tagtime timestamp --information \"[psst]I see metadata\" --url http://www.bumperdumper.com\n"
"  --meta-uuid tagr text \"Johnny Appleseed\" --meta-uuid \302\251sft text \"OpenShiiva encoded.\"\n"
"  --meta-uuid scan file /usr/pix/scans.zip\n"
"  --meta-uuid 1040 file ../../2006_taxes.pdf description=\"Fooled 'The Man' yet again.\"\n"
"can be removed with:\n"
"  --tagtime \"\" --information \"\"  --url \" \"  --meta-uuid scan file ""\n"
"  --manualAtomRemove \"moov.udta.meta.uuid=672c98cd-f11f-51fd-adec-b0ee7b4d215f\" \\\n"
"  --manualAtomRemove \"moov.udta.meta.uuid=1fed6656-d911-5385-9cb2-cb2c100f06e7\"\n"
"Remove the Sony uuid atoms with:\n"
"  --manualAtomRemove moov.trak[1].uuid=55534d54-21d2-4fce-bb88-695cfac9c740 \\\n"
"  --manualAtomRemove moov.trak[2].uuid=55534d54-21d2-4fce-bb88-695cfac9c740 \\\n"
"  --manualAtomRemove uuid=50524f46-21d2-4fce-bb88-695cfac9c740\n"
"\n"
"Viewing the contents of uuid atoms:\n"
"  -t or --textdata           Shows the uuid atoms (both text & file) that AP sets:\n"
"  Example output:\n"
"    Atom uuid=ec0f...d7 (AP uuid for \"scan\") contains: FILE.zip; description=[none]\n"
"    Atom uuid=672c...5f (AP uuid for \"tagr\") contains: Johnny Appleseed\n"
"\n"
"Extracting an embedded file in a uuid atom:\n"
"  --extract1uuid      (atom)           Extract file embedded within uuid=atom into same folder\n"
"                                        (file will be named with suffix shown in --textdata)\n"
"  --extract-uuids     [/path]          Extract all files in uuid atoms under the moov.udta.meta\n"
"                                         hierarchy. If no /path is given, files will be extracted\n"
"                                         to the same folder as the originating file.\n"
"\n"
" Examples:\n"
" --extract1uuid scan\n"
"  ...  Extracted uuid=scan attachment to file: /some/path/FILE_scan_uuid.zip\n"
" --extract-uuids ~/Desktop/plops\n"
"  ...  Extracted uuid=pass attachment to file: /Users/me/Desktop/plops_pass_uuid.pdf\n"
"  ...  Extracted uuid=site attachment to file: /Users/me/Desktop/plops_site_uuid.html\n"
"\n"
"------------------------------------------------------------------------------------------------\n"
;

static char* rDNSHelp_text =
"AtomicParsley help page for setting reverse domain '----' metadata atoms.\n"
"----------------------------------------------------------------------------------------------------\n"
"          Please note that the reverse DNS format supported here is not feature complete.\n"
"\n"
" Another style of metadata that iTunes uses is called the reverse DNS format. For all known tags,\n"
" iTunes offers no user-accessible exposure to these tags or their contents. This reverse DNS form has\n"
" a differnt form than other iTunes tags that have a atom name that describes the content of 'data'\n"
" atom it contains. In the reverseDNS format, the parent to the structure called the '----' atom, with\n"
" children atoms that describe & contain the metadata carried. The 'mean' child contains the reverse\n"
" domain itself ('com.apple.iTunes') & the 'name' child contains the descriptor ('iTunNORM'). A 'data'\n"
" atom follows that actually contains the contents of the tag.\n"
"\n"
"  --contentRating (rating)             Set the US TV/motion picture media content rating\n"
"                                         for available ratings use \"AtomicParsley --ratings-list\n"

"  --rDNSatom      (str)   name=(name_str) domain=(reverse_domain)  Manually set a reverseDNS atom.\n"
"\n"
" To set the form manually, 3 things are required: a domain, a name, and the desired text.\n"
" Note: multiple 'data' atoms are supported, but not in the com.apple.iTunes domain\n"
" Examples:\n"
"  --contentRating \"NC-17\" --contentRating \"TV-Y7\"\n"
"  --rDNSatom \"mpaa|PG-13|300|\" name=iTunEXTC domain=com.apple.iTunes\n"
"  --contentRating \"\"\n"
"  --rDNSatom \"\" name=iTunEXTC domain=com.apple.iTunes\n"
"  --rDNSatom \"try1\" name=EVAL domain=org.fsf --rDNSatom \"try 2\" name=EVAL domain=org.fsf\n"
"  --rDNSatom \"\" name=EVAL domain=org.fsf\n"
"----------------------------------------------------------------------------------------------------\n"
;

static char* ID3Help_text =
"AtomicParsley help page for ID32 atoms with ID3 tags.\n"
"----------------------------------------------------------------------------------------------------\n"
"      **  Please note: ID3 tag support is not feature complete & is in an alpha state.  **\n"
"----------------------------------------------------------------------------------------------------\n"
" ID3 tags are the tagging scheme used by mp3 files (where they are found typically at the start of the\n"
" file). In mpeg-4 files, ID3 version 2 tags are located in specific hierarchies at certain levels, at\n"
" file/movie/track level. The way that ID3 tags are carried on mpeg-4 files (carried by 'ID32' atoms)\n"
" was added in early 2006, but the ID3 tagging 'informal standard' was last updated (to v2.4) in 2000.\n"
" With few exceptions, ID3 tags in mpeg-4 files exist identically to their mp3 counterparts.\n"
"\n"
" The ID3 parlance, a frame contains an piece of metadata. A frame (like COMM for comment, or TIT1 for\n"
" title) contains the information, while the tag contains all the frames collectively. The 'informal\n"
" standard' for ID3 allows multiple langauges for frames like COMM (comment) & USLT (lyrics). In mpeg-4\n"
" this language setting is removed from the ID3 domain and exists in the mpeg-4 domain. That means that\n"
" when an english and a spanish comment are set, 2 separate ID32 atoms are created, each with a tag & 1\n"
" frame as in this example:\n" 
"       --ID3Tag COMM \"Primary\" --desc=AAA --ID3Tag COMM \"El Segundo\" UTF16LE lang=spa --desc=AAA\n"
" See available frames with \"AtomicParsley --ID3frames-list\"\n"
" See avilable imagetypes with \"AtomicParsley --imagetype-list\"\n"
"\n"
" AtomicParsley writes ID3 version 2.4.0 tags *only*. There is no up-converting from older versions.\n"
" Defaults are:\n"
"   default to movie level (moov.meta.ID32); other options are [ \"root\", \"track=(num)\" ] (see WARNING)\n"
"   UTF-8 text encoding when optional; other options are [ \"LATIN1\", \"UTF16BE\", \"UTF16LE\" ]\n"
"   frames that require descriptions have a default of \"\"\n"
"   for frames requiring a language setting, the ID32 language is used (currently defaulting to 'eng')\n"
"   frames that require descriptions have a default of \"\"\n"
"   image type defaults to 0x00 or Other; for image type 0x01, 32x32 png is enforced (switching to 0x02)\n"
"   setting the image mimetype is generally not required as the file is tested, but can be overridden\n"
"   zlib compression off\n"
"\n"
"  WARNING:\n"
"     Quicktime Player (up to v7.1.3 at least) will freeze opeing a file with ID32 tags at movie level.\n"
"     Specifically, the parent atom, 'meta' is the source of the issue. You can set the tags at file or\n"
"     track level which avoids the problem, but the default is movie level. iTunes is unaffected.\n"
"----------------------------------------------------------------------------------------------------\n"
"   Current limitations:\n"
"   - syncsafe integers are used as indicated by the id3 \"informal standard\". usage/reading of\n"
"     nonstandard ordinary unsigned integers (uint32_t) is not/will not be implemented.\n"
"   - externally referenced images (using mimetype '-->') are prohibited by the ID32 specification.\n"
"   - the ID32 atom is only supported in a non-referenced context\n"
"   - probably a raft of other limitations that my brain lost along the way...\n"
"----------------------------------------------------------------------------------------------------\n"
" Usage:\n"
"  --ID3Tag (frameID or alias) (str) [desc=(str)] [mimetype=(str)] [imagetype=(str or hex)] [...]\n"
"\n"
"  ... represents other arguments:\n"
"   [compressed] zlib compress the frame\n"
"   [UTF16BE, UTF16LE, LATIN1] alternative text encodings for frames that support different encodings\n"
"\n"
"Note: (foo) denotes required arguments; [foo] denotes optional parameters\n"
"\n"
" Examples:\n"
" --ID3Tag APIC /path/to/img.ext\n"
" --ID3Tag APIC /path/to/img.ext desc=\"something to say\" imagetype=0x08 UTF16LE compressed\n"
" --ID3Tag composer \"I, Claudius\" --ID3Tag TPUB \"Seneca the Roman\" --ID3Tag TMOO Imperial\n"
" --ID3Tag UFID look@me.org uniqueID=randomUUIDstamp\n"
"\n"
" Extracting embedded images in APIC frames:\n"
" --ID3Tag APIC extract\n"
"       images are extracted into the same directory as the source mpeg-4 file\n"
"\n"
#if defined (DARWIN_PLATFORM)
" Setting MCDI (Music CD Identifier):\n"
" --ID3Tag MCDI disk4\n"
"       Information to create this frame is taken directly off an Audio CD's TOC. If the target\n"
"       disk is not found or is not an audio CD, a scan of all devices will occur. If an Audio CD\n"
"       is present, the scan should yield what should be entered after 'MCDI':\n"
"          % AP file --ID3Tag MCDI disk3\n"
"          % No cd present in drive at disk3\n"
"          % Device 'disk4' contains cd media\n"
"          % Good news, device 'disk4' is an Audio CD and can be used for 'MCDI' setting\n"
#elif defined (HAVE_LINUX_CDROM_H)
" Setting MCDI (Music CD Identifier):\n"
" --ID3Tag MCDI /dev/hdc\n"
"       Information to create this frame is taken directly off an Audio CD's TOC. An Audio CD\n"
"       must be mounted & present.\n"
#elif defined (WIN32)
" Setting MCDI (Music CD Identifier):\n"
" --ID3Tag MCDI D\n"
"       Information to create this frame is taken directly off an Audio CD's TOC. The letter after\n"
"       \"MCDI\" is the letter of the drive where the CD is present.\n"
#endif
;

void ExtractPaddingPrefs(char* env_padding_prefs) {
	pad_prefs.default_padding_size = DEFAULT_PADDING_LENGTH;
	pad_prefs.minimum_required_padding_size = MINIMUM_REQUIRED_PADDING_LENGTH;
	pad_prefs.maximum_present_padding_size = MAXIMUM_REQUIRED_PADDING_LENGTH;
	
	if (env_padding_prefs != NULL) {
		if (env_padding_prefs[0] == 0x22 || env_padding_prefs[0] == 0x27) env_padding_prefs++;
	}
	char* env_pad_prefs_ptr = env_padding_prefs;
	
	while (env_pad_prefs_ptr != NULL) {
		env_pad_prefs_ptr = strsep(&env_padding_prefs,":");
		
		if (env_pad_prefs_ptr == NULL) break;
		
		if (memcmp(env_pad_prefs_ptr, "DEFAULT_PAD=", 12) == 0) {
			strsep(&env_pad_prefs_ptr,"=");
			sscanf(env_pad_prefs_ptr, "%u", &pad_prefs.default_padding_size);
		}
		if (memcmp(env_pad_prefs_ptr, "MIN_PAD=", 8) == 0) {
			strsep(&env_pad_prefs_ptr,"=");
			sscanf(env_pad_prefs_ptr, "%u", &pad_prefs.minimum_required_padding_size);
		}
		if (memcmp(env_pad_prefs_ptr, "MAX_PAD=", 8) == 0) {
			strsep(&env_pad_prefs_ptr,"=");
			sscanf(env_pad_prefs_ptr, "%u", &pad_prefs.maximum_present_padding_size);
		}
	}
	//fprintf(stdout, "Def %u; Min %u; Max %u\n", pad_prefs.default_padding_size, pad_prefs.minimum_required_padding_size, pad_prefs.maximum_present_padding_size);
	return;
}

void GetBasePath(const char *filepath, char* &basepath) {
	//with a myriad of m4a, m4p, mp4, whatever else comes up... it might just be easiest to strip off the end.
	int split_here = 0;
	for (int i=strlen(filepath); i >= 0; i--) {
		const char* this_char=&filepath[i];
		if ( strncmp(this_char, ".", 1) == 0 ) {
			split_here = i;
			break;
		}
	}
	memcpy(basepath, filepath, (size_t)split_here);
	
	return;
}

void find_optional_args(char *argv[], int start_optindargs, uint16_t &packed_lang, bool &asUTF16, uint8_t &udta_container, uint8_t &trk_idx, int max_optargs) {
	asUTF16 = false;
	packed_lang = 5575; //und = 0x55C4 = 21956, but QTPlayer doesn't like und //eng =  0x15C7 = 5575
	
	for (int i= 0; i <= max_optargs-1; i++) {
		if ( argv[start_optindargs + i] && start_optindargs + i <= total_args ) {
			if ( memcmp(argv[start_optindargs + i], "lang=", 5) == 0 ) {
				if (!MatchLanguageCode(argv[start_optindargs +i]+5) ) {
					packed_lang = PackLanguage("und", 0);
				} else {
					packed_lang = PackLanguage(argv[start_optindargs +i], 5);
				}
			
			} else if ( memcmp(argv[start_optindargs + i], "UTF16", 5) == 0 ) {
				asUTF16 = true;
			} else if ( memcmp(argv[optind + i], "movie", 6) == 0 ) {
				udta_container = MOVIE_LEVEL_ATOM;
			} else if ( memcmp(argv[optind + i], "track=", 6) == 0 ) {
				char* track_index_str = argv[optind + i];
				strsep(&track_index_str, "=");
				sscanf(track_index_str, "%hhu", &trk_idx);
				udta_container = SINGLE_TRACK_ATOM;	
			} else if ( memcmp(argv[optind + i], "track", 6) == 0 ) {
				udta_container = ALL_TRACKS_ATOM;
			}
			if (memcmp(argv[start_optindargs + i], "-", 1) == 0) {
				break; //we've hit another cli argument
			}
		}
	}
	return;
}

void scan_ID3_optargs(char *argv[], int start_optargs, char* &target_lang, uint16_t &packed_lang, uint8_t &char_encoding, char* meta_container, bool &multistring) {
	packed_lang = 5575; //default ID32 lang is 'eng'
	uint16_t i = 0;
	
	while (argv[start_optargs + i] != NULL) {
		if ( argv[start_optargs + i] && start_optargs + i <= total_args ) {
			
			if ( memcmp(argv[start_optargs + i], "lang=", 5) == 0 ) {
				if (!MatchLanguageCode(argv[start_optargs +i]+5) ) {
					packed_lang = PackLanguage("und", 0);
					target_lang = "und";
				} else {
					packed_lang = PackLanguage(argv[start_optargs +i], 5);
					target_lang = argv[start_optargs + i] + 5;
				}
			
			} else if ( memcmp(argv[start_optargs + i], "UTF16LE", 8) == 0 ) {
				char_encoding = TE_UTF16LE_WITH_BOM;
			} else if ( memcmp(argv[start_optargs + i], "UTF16BE", 8) == 0 ) {
				char_encoding = TE_UTF16BE_NO_BOM;
			} else if ( memcmp(argv[start_optargs + i], "LATIN1", 7) == 0 ) {
				char_encoding = TE_LATIN1;
				
			} else if ( memcmp(argv[optind + i], "root", 5) == 0 ) {
				*meta_container = 0-FILE_LEVEL_ATOM;
			} else if ( memcmp(argv[optind + i], "track=", 6) == 0 ) {
				char* track_index_str = argv[optind + i];
				strsep(&track_index_str, "=");
				sscanf(track_index_str, "%hhu", meta_container);
			}
		}
		
		if (memcmp(argv[start_optargs + i], "-", 1) == 0) {
			break; //we've hit another cli argument or deleting some frame
		}
		i++;
	}
	return;
}

char* find_ID3_optarg(char *argv[], int start_optargs, char* arg_string) {
	char* ret_val = "";
	uint16_t i = 0;
	uint8_t arg_prefix_len = strlen(arg_string);
	
	while (argv[start_optargs + i] != NULL) {
		if ( argv[start_optargs + i] && start_optargs + i <= total_args ) {
			if (memcmp(arg_string, "compressed", 11) == 0 && memcmp(argv[start_optargs + i], "compressed", 11) == 0) {
				return "1";
			}
			//if (memcmp(arg_string, "text++", 7) == 0 && memcmp(argv[start_optargs + i], "text++", 7) == 0) {
			//	return "1";
			//}
			if (memcmp(argv[start_optargs + i], arg_string, arg_prefix_len) == 0) {
				ret_val = argv[start_optargs + i] + arg_prefix_len;
				break;
			}
		}
		if (memcmp(argv[start_optargs + i], "-", 1) == 0) {
			break; //we've hit another cli argument or deleting some frame
		}
		i++;
	}
	return ret_val;
}

//***********************************************

#if defined (_MSC_VER)
int wmain( int argc, wchar_t *arguments[]) {
	uint16_t name_len = wcslen(arguments[0]);
	if (wmemcmp(arguments[0] + (name_len-9), L"-utf8.exe", 9) == 0 || wmemcmp(arguments[0] + (name_len-9), L"-UTF8.exe", 9) == 0) {
		UnicodeOutputStatus = UNIVERSAL_UTF8;
	} else {
		UnicodeOutputStatus = WIN32_UTF16;
	}

	char *argv[350];
	//for native Win32 & full unicode support (well, cli arguments anyway), take whatever 16bit unicode windows gives (utf16le), and convert EVERYTHING
	//that is sends to utf8; use those utf8 strings (mercifully subject to familiar standby's like strlen) to pass around the program like getopt_long
	//to get cli options; convert from utf8 filenames as required for unicode filename support on Windows using wide file functions. Here, EVERYTHING = 350.
	for(int z=0; z < argc; z++) {
		uint32_t wchar_length = wcslen(arguments[z])+1;
		argv[z] = (char *)malloc(sizeof(char)*8*wchar_length );
		memset(argv[z], 0, 8*wchar_length);
		if (UnicodeOutputStatus == WIN32_UTF16) {
			UTF16LEToUTF8((unsigned char*) argv[z], 8*wchar_length, (unsigned char*) arguments[z], wchar_length*2);
		} else {
			strip_bogusUTF16toRawUTF8((unsigned char*) argv[z], 8*wchar_length, arguments[z], wchar_length );
		}
	}
	argv[argc] = NULL;

#else
int main( int argc, char *argv[]) {
#endif
	if (argc == 1) {
		fprintf (stdout,"%s\n", shortHelp_text); exit(0);
	} else if (argc == 2 && ((strncmp(argv[1],"-v",2) == 0) || (strncmp(argv[1],"-version",2) == 0)) ) {
	
		ShowVersionInfo();
		exit(0);
		
	} else if (argc == 2) {
		if ( (strncmp(argv[1],"-help",5) == 0) || (strncmp(argv[1],"--help",6) == 0) || (strncmp(argv[1],"-h",5) == 0 ) ) {
			fprintf(stdout, "%s\n", shortHelp_text); exit(0);
			
		} else if ( (strncmp(argv[1],"--longhelp", 10) == 0) || (strncmp(argv[1],"-longhelp", 9) == 0) || (strncmp(argv[1],"-Lh", 3) == 0) ) {
			if (UnicodeOutputStatus == WIN32_UTF16) { //convert the helptext to utf16 to preserve © characters
				int help_len = strlen(longHelp_text)+1;
				wchar_t* Lhelp_text = (wchar_t *)malloc(sizeof(wchar_t)*help_len);
				wmemset(Lhelp_text, 0, help_len);
				UTF8ToUTF16LE((unsigned char*)Lhelp_text, 2*help_len, (unsigned char*)longHelp_text, help_len);
#if defined (_MSC_VER)
				APar_unicode_win32Printout(Lhelp_text, longHelp_text);
#endif
				free(Lhelp_text);
			} else {
				fprintf(stdout, "%s", longHelp_text);
			}
			exit(0);
			
		} else if ( (strncmp(argv[1],"--3gp-help", 10) == 0) || (strncmp(argv[1],"-3gp-help", 9) == 0) || (strncmp(argv[1],"--3gp-h", 7) == 0) ) {
			fprintf(stdout, "%s\n", _3gpHelp_text); exit(0);
			
		} else if ( (strncmp(argv[1],"--ISO-help", 10) == 0) || (strncmp(argv[1],"--iso-help", 10) == 0) || (strncmp(argv[1],"-Ih", 3) == 0) ) {
			fprintf(stdout, "%s\n", ISOHelp_text); exit(0);
			
		} else if ( (strncmp(argv[1],"--file-help", 11) == 0) || (strncmp(argv[1],"-file-help", 10) == 0) || (strncmp(argv[1],"-fh", 3) == 0) ) {
			fprintf(stdout, "%s\n", fileLevelHelp_text); exit(0);
			
		} else if ( (strncmp(argv[1],"--uuid-help", 11) == 0) || (strncmp(argv[1],"-uuid-help", 10) == 0) || (strncmp(argv[1],"-uh", 3) == 0) ) {
			fprintf(stdout, "%s\n", uuidHelp_text); exit(0);
			
		} else if ( (strncmp(argv[1],"--reverseDNS-help", 18) == 0) || (strncmp(argv[1],"-rDNS-help", 10) == 0) || (strncmp(argv[1],"-rh", 3) == 0) ) {
			fprintf(stdout, "%s\n", rDNSHelp_text); exit(0);
			
		} else if ( (strncmp(argv[1],"--ID3-help", 10) == 0) || (strncmp(argv[1],"-ID3-help", 9) == 0) || (strncmp(argv[1],"-ID3h", 4) == 0) ) {
			fprintf(stdout, "%s\n", ID3Help_text); exit(0);
			
		} else if ( memcmp(argv[1], "--genre-list", 12) == 0 ) {
			ListGenresValues(); exit(0);
			
		} else if ( memcmp(argv[1], "--stik-list", 11) == 0 ) {
			ListStikValues(); exit(0);
			
		} else if ( memcmp(argv[1], "--language-list", 16) == 0 ||
								memcmp(argv[1], "--languages-list", 17) == 0 ||
								memcmp(argv[1], "--list-language", 16) == 0 ||
								memcmp(argv[1], "--list-languages", 17) == 0 ||
								memcmp(argv[1], "-ll", 3) == 0) {
			ListLanguageCodes(); exit(0);

		} else if (memcmp(argv[1], "--ratings-list", 14) == 0) {
			ListMediaRatings(); exit(0);
			
		} else if (memcmp(argv[1], "--ID3frames-list", 17) == 0) {
			ListID3FrameIDstrings(); exit(0);
		
		} else if (memcmp(argv[1], "--imagetype-list", 17) == 0) {
			List_imagtype_strings(); exit(0);
		}
	}
	
	if ( argc == 3 && (memcmp(argv[2], "--brands", 8) == 0 || memcmp(argv[2], "-brands", 7) == 0) ) {
		APar_ExtractBrands(argv[1]); exit(0);
	}
	
	int extr = 99;
	total_args = argc;
	char* ISObasemediafile = argv[1];
	
	TestFileExistence(ISObasemediafile, true);
	xmlInitEndianDetection(); 
	
	char* padding_options = getenv("AP_PADDING");
	ExtractPaddingPrefs(padding_options);
	
	//it would probably be better to test output_file if provided & if --overWrite was provided.... probably only of use on Windows - and I'm not on it.
	if (strlen(ISObasemediafile) + 11 > MAXPATHLEN) {
		fprintf(stderr, "%c %s", '\a', "AtomicParsley error: filename/filepath was too long.\n");
		exit(1);
	}
	
	if ( argc > 3 && memcmp(argv[2], "--DeepScan", 10) == 0) {
		deep_atom_scan = true;
		APar_ScanAtoms(ISObasemediafile, true);
	}
	
	while (1) {
	static struct option long_options[] = {
		{ "help",						  0,									NULL,						OPT_HELP },
		{ "test",					  	optional_argument,	NULL,						OPT_TEST },
		{ "textdata",         optional_argument,  NULL,           OPT_ShowTextData },
		{ "extractPix",				0,									NULL,           OPT_ExtractPix },
		{ "extractPixToPath", required_argument,	NULL,				    OPT_ExtractPixToPath },
		{ "artist",           required_argument,  NULL,						Meta_artist },
		{ "title",            required_argument,  NULL,						Meta_songtitle },
		{ "album",            required_argument,  NULL,						Meta_album },
		{ "genre",            required_argument,  NULL,						Meta_genre },
		{ "tracknum",         required_argument,  NULL,						Meta_tracknum },
		{ "disknum",          required_argument,  NULL,						Meta_disknum },
		{ "comment",          required_argument,  NULL,						Meta_comment },
		{ "year",             required_argument,  NULL,						Meta_year },
		{ "lyrics",           required_argument,  NULL,						Meta_lyrics },
		{ "composer",         required_argument,  NULL,						Meta_composer },
		{ "copyright",        required_argument,  NULL,						Meta_copyright },
		{ "grouping",         required_argument,  NULL,						Meta_grouping },
		{ "albumArtist",      required_argument,  NULL,           Meta_album_artist },
    { "compilation",      required_argument,  NULL,						Meta_compilation },
		{ "advisory",         required_argument,  NULL,						Meta_advisory },
    { "bpm",              required_argument,  NULL,						Meta_BPM },
		{ "artwork",          required_argument,  NULL,						Meta_artwork },
		{ "stik",             required_argument,  NULL,           Meta_stik },
    { "description",      required_argument,  NULL,           Meta_description },
    { "TVNetwork",        required_argument,  NULL,           Meta_TV_Network },
    { "TVShowName",       required_argument,  NULL,           Meta_TV_ShowName },
    { "TVEpisode",        required_argument,  NULL,           Meta_TV_Episode },
    { "TVEpisodeNum",     required_argument,  NULL,           Meta_TV_EpisodeNumber },
    { "TVSeasonNum",      required_argument,  NULL,           Meta_TV_SeasonNumber },		
		{ "podcastFlag",      required_argument,  NULL,           Meta_podcastFlag },
		{ "keyword",          required_argument,  NULL,           Meta_keyword },
		{ "category",         required_argument,  NULL,           Meta_category },
		{ "podcastURL",       required_argument,  NULL,           Meta_podcast_URL },
		{ "podcastGUID",      required_argument,  NULL,           Meta_podcast_GUID },
		{ "purchaseDate",     required_argument,  NULL,           Meta_PurchaseDate },
		{ "encodingTool",     required_argument,  NULL,           Meta_EncodingTool },
		{ "gapless",          required_argument,  NULL,           Meta_PlayGapless },
		{ "sortOrder",        required_argument,  NULL,           Meta_SortOrder } ,
		
		{ "rDNSatom",         required_argument,  NULL,           Meta_ReverseDNS_Form },
		{ "contentRating",    required_argument,  NULL,           Meta_rDNS_rating },
		
		{ "tagtime",          optional_argument,  NULL,						Meta_StandardDate },
		{ "information",      required_argument,  NULL,           Meta_Information },
		{ "url",              required_argument,  NULL,           Meta_URL },
		{ "meta-uuid",        required_argument,  NULL,           Meta_uuid },
		{ "extract-uuids",    optional_argument,  NULL,           Opt_Extract_all_uuids },
		{ "extract1uuid",     required_argument,  NULL,           Opt_Extract_a_uuid },
		{ "iPod-uuid",        required_argument,  NULL,           Opt_Ipod_AVC_uuid },
		
		{ "freefree",         optional_argument,  NULL,           Opt_FreeFree },
		{ "metaEnema",        0,                  NULL,						Metadata_Purge },
		{ "manualAtomRemove", required_argument,  NULL,           Manual_atom_removal },
		{ "udtaEnema",        0,                  NULL,           UserData_Purge },
		{ "foobar2000Enema",  0,                  NULL,           foobar_purge },
		{ "metaDump",         0,                  NULL,						Meta_dump },
		{ "output",           required_argument,  NULL,						OPT_OutputFile },
		{ "preventOptimizing",0,                  NULL,						OPT_NoOptimize },
		{ "overWrite",        0,                  NULL,						OPT_OverWrite },
		
		{ "ISO-copyright",    required_argument,  NULL,						ISO_Copyright },
		
		{ "3gp-title",        required_argument,  NULL,           _3GP_Title },
		{ "3gp-author",       required_argument,  NULL,           _3GP_Author },
		{ "3gp-performer",    required_argument,  NULL,           _3GP_Performer },
		{ "3gp-genre",        required_argument,  NULL,           _3GP_Genre },
		{ "3gp-description",  required_argument,  NULL,           _3GP_Description },
		{ "3gp-copyright",    required_argument,  NULL,           _3GP_Copyright },
		{ "3gp-album",        required_argument,  NULL,           _3GP_Album },
		{ "3gp-year",         required_argument,  NULL,           _3GP_Year },
		
		{ "3gp-rating",       required_argument,  NULL,           _3GP_Rating },
		{ "3gp-classification",  required_argument,  NULL,           _3GP_Classification },
		{ "3gp-keyword",      required_argument,  NULL,           _3GP_Keyword },
		{ "3gp-location",     required_argument,  NULL,           _3GP_Location },
		
		{ "ID3Tag",           required_argument,  NULL,           Meta_ID3v2Tag },
		
		{ "DeepScan",         0,                  &extr,          1 },
		
		{ 0, 0, 0, 0 }
	};
		
	int c = -1;
	int option_index = 0; 
	
	c = getopt_long(argc, argv, "hTtEe:a:b:c:d:f:g:i:k:l:n:o:pq::u:w:y:z:A:B:C:D:F:G:H:I:J:K:L:MN:QR:S:U:WXV:ZP 0xAA: 0xAB: 0xAC: 0xAD: 0xAE: 0xAF: 0xB0: 0xB1: 0xB2: 0xB3: 0xB4: 0xB5: 0xB6:", long_options, &option_index);
	
	if (c == -1) {
		if (argc < 3 && argc > 2) {
			APar_ScanAtoms(ISObasemediafile, true);
			APar_PrintAtomicTree();
		}
		break;
	}
	
	signal(SIGTERM, kill_signal);
#ifndef WIN32
	signal(SIGKILL, kill_signal);
#endif
	signal(SIGINT,  kill_signal);
	
	switch(c) {
		// "optind" represents the count of arguments up to and including its optional flag:

		case '?': return 1;
			
		case OPT_HELP: {
			fprintf (stdout,"%s", longHelp_text); return 0;
		}
					
		case OPT_TEST: {
			deep_atom_scan = true;
			APar_ScanAtoms(ISObasemediafile, true);
			APar_PrintAtomicTree();
			if (argv[optind]) {
				if (memcmp(argv[optind], "+dates", 6) == 0) {
					APar_ExtractDetails( APar_OpenISOBaseMediaFile(ISObasemediafile, true), SHOW_TRACK_INFO + SHOW_DATE_INFO );
				} else {
					APar_ExtractDetails( APar_OpenISOBaseMediaFile(ISObasemediafile, true), SHOW_TRACK_INFO);
				}
			}
			break;
		}
		
		case OPT_ShowTextData: {
			if (argv[optind]) { //for utilities that write iTunes-style metadata into 3gp branded files
				APar_ExtractBrands(ISObasemediafile);
				deep_atom_scan=true;
				APar_ScanAtoms(ISObasemediafile);
				
				APar_OpenISOBaseMediaFile(ISObasemediafile, true);
				
				if (memcmp(argv[optind], "+", 1) == 0) {
					APar_Print_iTunesData(ISObasemediafile, NULL, PRINT_FREE_SPACE + PRINT_PADDING_SPACE + PRINT_USER_DATA_SPACE + PRINT_MEDIA_SPACE, PRINT_DATA );
				} else {
					fprintf(stdout, "---------------------------\n");
					APar_Print_ISO_UserData_per_track();
					
					AtomicInfo* iTuneslistAtom = APar_FindAtom("moov.udta.meta.ilst", false, SIMPLE_ATOM, 0);
					if (iTuneslistAtom != NULL) {
						fprintf(stdout, "---------------------------\n  iTunes-style metadata tags:\n");
						APar_Print_iTunesData(ISObasemediafile, NULL, PRINT_FREE_SPACE + PRINT_PADDING_SPACE + PRINT_USER_DATA_SPACE + PRINT_MEDIA_SPACE, PRINT_DATA, iTuneslistAtom );
					}
					fprintf(stdout, "---------------------------\n");
				}
				
			} else {
				deep_atom_scan=true;
				APar_ScanAtoms(ISObasemediafile);
				APar_OpenISOBaseMediaFile(ISObasemediafile, true);
				
				if (metadata_style >= THIRD_GEN_PARTNER) {
					APar_PrintUserDataAssests();
				} else if (metadata_style == ITUNES_STYLE) {
					APar_Print_iTunesData(ISObasemediafile, NULL, 0, PRINT_DATA); //false, don't try to extractPix
					APar_Print_APuuid_atoms(ISObasemediafile, NULL, PRINT_DATA);
				}
			}
			APar_OpenISOBaseMediaFile(ISObasemediafile, false);
			APar_FreeMemory();
			break;
		}
					
		case OPT_ExtractPix: {
			char* base_path=(char*)malloc(sizeof(char)*MAXPATHLEN+1);
			memset(base_path, 0, MAXPATHLEN +1);
			
			GetBasePath( ISObasemediafile, base_path );
			APar_ScanAtoms(ISObasemediafile);
			APar_OpenISOBaseMediaFile(ISObasemediafile, true);
			APar_Print_iTunesData(ISObasemediafile, base_path, 0, EXTRACT_ARTWORK); //exportPix to stripped ISObasemediafile path
			APar_OpenISOBaseMediaFile(ISObasemediafile, false);
			
			free(base_path);
			base_path = NULL;
			break;
		}
		
		case OPT_ExtractPixToPath: {
			APar_ScanAtoms(ISObasemediafile);
			APar_OpenISOBaseMediaFile(ISObasemediafile, true);
			APar_Print_iTunesData(ISObasemediafile, optarg, 0, EXTRACT_ARTWORK); //exportPix to a different path
			APar_OpenISOBaseMediaFile(ISObasemediafile, false);
			break;
		}
				
		case Meta_artist : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "artist") ) {
				char major_brand[4];
				UInt32_TO_String4(brand, &*major_brand);
				APar_assert(false, 4, &*major_brand);
				break;
			}
			AtomicInfo* artistData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.©ART.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(artistData_atom, optarg, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			break;
		}
		
		case Meta_songtitle : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "title") ) {
				break;
			}
			
			AtomicInfo* titleData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.©nam.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(titleData_atom, optarg, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			break;
		}
		
		case Meta_album : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "album") ) {
				break;
			}
			
			AtomicInfo* albumData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.©alb.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(albumData_atom, optarg, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			break;
		}
		
		case Meta_genre : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "genre") ) {
				break;
			}
			
			APar_MetaData_atomGenre_Set(optarg);
			break;
		}
				
		case Meta_tracknum : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "track number") ) {
				break;
			}
			
			uint16_t pos_in_total = 0;
			uint16_t the_total = 0; 
			if (strrchr(optarg, '/') != NULL) {

				char* duplicate_info = optarg;
				char* item_stat = strsep(&duplicate_info,"/");
				sscanf(item_stat, "%hu", &pos_in_total); //sscanf into a an unsigned char (uint8_t is typedef'ed to a unsigned char by gcc)
				item_stat = strsep(&duplicate_info,"/");
				sscanf(item_stat, "%hu", &the_total);
			} else {
				sscanf(optarg, "%hu", &pos_in_total);
			}
			
			AtomicInfo* tracknumData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.trkn.data", optarg, AtomFlags_Data_Binary);
			//tracknum: [0, 0, 0, 0,   0, 0, 0, pos_in_total, 0, the_total, 0, 0]; BUT that first uint32_t is already accounted for in APar_MetaData_atom_Init
			APar_Unified_atom_Put(tracknumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 16);
			APar_Unified_atom_Put(tracknumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, pos_in_total, 16);
			APar_Unified_atom_Put(tracknumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, the_total, 16);
			APar_Unified_atom_Put(tracknumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 16);
			break;
		}
		
		case Meta_disknum : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "disc number") ) {
				break;
			}
			
			uint16_t pos_in_total = 0;
			uint16_t the_total = 0;
			if (strrchr(optarg, '/') != NULL) {
				
				char* duplicate_info = optarg;
				char* item_stat = strsep(&duplicate_info,"/");
				sscanf(item_stat, "%hu", &pos_in_total); //sscanf into a an unsigned char (uint8_t is typedef'ed to a unsigned char by gcc)
				item_stat = strsep(&duplicate_info,"/");
				sscanf(item_stat, "%hu", &the_total);
			
			} else {
				sscanf(optarg, "%hu", &pos_in_total);
			}
			
			AtomicInfo* disknumData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.disk.data", optarg, AtomFlags_Data_Binary);
			//disknum: [0, 0, 0, 0,   0, 0, 0, pos_in_total, 0, the_total]; BUT that first uint32_t is already accounted for in APar_MetaData_atom_Init
			APar_Unified_atom_Put(disknumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 16);
			APar_Unified_atom_Put(disknumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, pos_in_total, 16);
			APar_Unified_atom_Put(disknumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, the_total, 16);
			break;
		}
		
		case Meta_comment : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "comment") ) {
				break;
			}
			
			AtomicInfo* commentData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.©cmt.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(commentData_atom, optarg, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			break;
		}
		
		case Meta_year : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "year") ) {
				break;
			}
			
			AtomicInfo* yearData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.©day.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(yearData_atom, optarg, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			break;
		}
		
		case Meta_lyrics : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "lyrics") ) {
				break;
			}
			
			AtomicInfo* lyricsData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.©lyr.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(lyricsData_atom, optarg, UTF8_iTunesStyle_Unlimited, 0, 0);
			break;
		}
		
		case Meta_composer : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "composer") ) {
				break;
			}
			
			AtomicInfo* composerData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.©wrt.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(composerData_atom, optarg, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			break;
		}
		
		case Meta_copyright : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "copyright") ) {
				break;
			}
			
			AtomicInfo* copyrightData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.cprt.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(copyrightData_atom, optarg, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			break;
		}
		
		case Meta_grouping : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "grouping") ) {
				break;
			}
			
			AtomicInfo* groupingData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.©grp.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(groupingData_atom, optarg, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			break;
		}
		
		case Meta_compilation : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "compilation") ) {
				break;
			}
			
			if (strncmp(optarg, "false", 5) == 0 || strlen(optarg) == 0) {
				APar_RemoveAtom("moov.udta.meta.ilst.cpil.data", VERSIONED_ATOM, 0);
			} else {
				//compilation: [0, 0, 0, 0,   boolean_value]; BUT that first uint32_t is already accounted for in APar_MetaData_atom_Init
				AtomicInfo* compilationData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.cpil.data", optarg, AtomFlags_Data_UInt);
				APar_Unified_atom_Put(compilationData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 1, 8); //a hard coded uint8_t of: 1 is compilation
			}
			break;
		}
		
		case Meta_BPM : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "BPM") ) {
				break;
			}
			
			if (strncmp(optarg, "0", 1) == 0 || strlen(optarg) == 0) {
				APar_RemoveAtom("moov.udta.meta.ilst.tmpo.data", VERSIONED_ATOM, 0);
			} else {
				uint16_t bpm_value = 0;
				sscanf(optarg, "%hu", &bpm_value );
				//bpm is [0, 0, 0, 0,   0, bpm_value]; BUT that first uint32_t is already accounted for in APar_MetaData_atom_Init
				AtomicInfo* bpmData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.tmpo.data", optarg, AtomFlags_Data_UInt);
				APar_Unified_atom_Put(bpmData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, bpm_value, 16);
			}
			break;
		}
		
		case Meta_advisory : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "content advisory") ) {
				break;
			}
			
			if (strncmp(optarg, "remove", 6) == 0 || strlen(optarg) == 0) {
				APar_RemoveAtom("moov.udta.meta.ilst.rtng.data", VERSIONED_ATOM, 0);
			} else {
				uint8_t rating_value = 0;
				if (strncmp(optarg, "clean", 5) == 0) {
					rating_value = 2; //only \02 is clean
				} else if (strncmp(optarg, "explicit", 8) == 0) {
					rating_value = 4; //most non \00, \02 numbers are allowed
				}
				//rating is [0, 0, 0, 0,   rating_value]; BUT that first uint32_t is already accounted for in APar_MetaData_atom_Init
				AtomicInfo* advisoryData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.rtng.data", optarg, AtomFlags_Data_UInt);
				APar_Unified_atom_Put(advisoryData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, rating_value, 8);
			}
			break;
		}
		
		case Meta_artwork : { //handled differently: there can be multiple "moov.udta.meta.ilst.covr.data" atoms
			char* env_PicOptions = getenv("PIC_OPTIONS");
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "coverart") ) {
				break;
			}
			
			APar_MetaData_atomArtwork_Set(optarg, env_PicOptions);
			break;
		}
				
		case Meta_stik : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "'stik'") ) {
				break;
			}
			
			if (strncmp(optarg, "remove", 6) == 0 || strlen(optarg) == 0) {
				APar_RemoveAtom("moov.udta.meta.ilst.stik.data", VERSIONED_ATOM, 0);
			} else {
				uint8_t stik_value = 0;
				
				if (memcmp(optarg, "value=", 6) == 0) {
					char* stik_val_str_ptr = optarg;
					strsep(&stik_val_str_ptr,"=");
					sscanf(stik_val_str_ptr, "%hhu", &stik_value);
				} else {
					stiks* return_stik = MatchStikString(optarg);
					if (return_stik != NULL) {
						stik_value = return_stik->stik_number;
						if (memcmp(optarg, "Audiobook", 9) == 0) {
							forced_suffix_type = FORCE_M4B_TYPE;
						}
					}
				}
				//stik is [0, 0, 0, 0,   stik_value]; BUT that first uint32_t is already accounted for in APar_MetaData_atom_Init
				AtomicInfo* stikData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.stik.data", optarg, AtomFlags_Data_UInt);
				APar_Unified_atom_Put(stikData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, stik_value, 8);
			}
			break;
		}
		
		case Meta_EncodingTool : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "encoding tool") ) {
				break;
			}
			
			AtomicInfo* encodingtoolData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.©too.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(encodingtoolData_atom, optarg, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			break;
		}
		
		case Meta_description : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "description") ) {
				break;
			}
			
			AtomicInfo* descriptionData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.desc.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(descriptionData_atom, optarg, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			break;
		}
		
		case Meta_TV_Network : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "TV Network") ) {
				break;
			}
			
			AtomicInfo* tvnetworkData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.tvnn.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(tvnetworkData_atom, optarg, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			break;
		}
		
		case Meta_TV_ShowName : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "TV Show name") ) {
				break;
			}
			
			AtomicInfo* tvshownameData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.tvsh.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(tvshownameData_atom, optarg, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			break;
		}
		
		case Meta_TV_Episode : { //if the show "ABC Lost 209", its "209"
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "TV Episode string") ) {
				break;
			}
			
			AtomicInfo* tvepisodeData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.tven.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(tvepisodeData_atom, optarg, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			break;
		}
		
		case Meta_TV_SeasonNumber : { //if the show "ABC Lost 209", its 2; integer 2 not char "2"
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "TV Season") ) {
				break;
			}
			
			uint16_t data_value = 0;
			sscanf(optarg, "%hu", &data_value );
			
			AtomicInfo* tvseasonData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.tvsn.data", optarg, AtomFlags_Data_UInt);
			//season is [0, 0, 0, 0,   0, 0, 0, data_value]; BUT that first uint32_t is already accounted for in APar_MetaData_atom_Init
			APar_Unified_atom_Put(tvseasonData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 16);
			APar_Unified_atom_Put(tvseasonData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, data_value, 16);
			break;
		}
		
		case Meta_TV_EpisodeNumber : { //if the show "ABC Lost 209", its 9; integer 9 (0x09) not char "9"(0x39)
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "TV Episode number") ) {
				break;
			}
			
			uint16_t data_value = 0;
			sscanf(optarg, "%hu", &data_value );
			
			AtomicInfo* tvepisodenumData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.tves.data", optarg, AtomFlags_Data_UInt);
			//episodenumber is [0, 0, 0, 0,   0, 0, 0, data_value]; BUT that first uint32_t is already accounted for in APar_MetaData_atom_Init
			APar_Unified_atom_Put(tvepisodenumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 16);
			APar_Unified_atom_Put(tvepisodenumData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, data_value, 16);
			break;
		}
		
		case Meta_album_artist : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "album artist") ) {
				break;
			}
			
			AtomicInfo* albumartistData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.aART.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(albumartistData_atom, optarg, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			break;
		}
		
		case Meta_podcastFlag : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "podcast flag") ) {
				break;
			}
			
			if (strncmp(optarg, "false", 5) == 0) {
				APar_RemoveAtom("moov.udta.meta.ilst.pcst.data", VERSIONED_ATOM, 0);
			} else {
				//podcastflag: [0, 0, 0, 0,   boolean_value]; BUT that first uint32_t is already accounted for in APar_MetaData_atom_Init
				AtomicInfo* podcastFlagData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.pcst.data", optarg, AtomFlags_Data_UInt);
				APar_Unified_atom_Put(podcastFlagData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 1, 8); //a hard coded uint8_t of: 1 denotes podcast flag
			}
			
			break;
		}
		
		case Meta_keyword : {    //TODO to the end of iTunes-style metadata & uuid atoms
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "keyword") ) {
				break;
			}
			
			AtomicInfo* keywordData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.keyw.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(keywordData_atom, optarg, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			break;
		}
		
		case Meta_category : { // see http://www.apple.com/itunes/podcasts/techspecs.html for available categories
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "category") ) {
				break;
			}
			
			AtomicInfo* categoryData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.catg.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(categoryData_atom, optarg, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			break;
		}
		
		case Meta_podcast_URL : { // usually a read-only value, but useful for getting videos into the 'podcast' menu
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "podcast URL") ) {
				break;
			}
			
			AtomicInfo* podcasturlData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.purl.data", optarg, AtomFlags_Data_Binary);
			APar_Unified_atom_Put(podcasturlData_atom, optarg, UTF8_iTunesStyle_Binary, 0, 0);
			break;
		}
		
		case Meta_podcast_GUID : { // Global Unique IDentifier; it is *highly* doubtful that this would be useful...
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "podcast GUID") ) {
				break;
			}
			
			AtomicInfo* globalidData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.egid.data", optarg, AtomFlags_Data_Binary);
			APar_Unified_atom_Put(globalidData_atom, optarg, UTF8_iTunesStyle_Binary, 0, 0);
			break;
		}
		
		case Meta_PurchaseDate : { // might be useful to *remove* this, but adding it... although it could function like id3v2 tdtg...
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "purchase date") ) {
				break;
			}
			char* purd_time;
			bool free_memory = false;
			if (optarg != NULL) {
				if (strncmp(optarg, "timestamp", 9) == 0) {
					purd_time = (char *)malloc(sizeof(char)*255);
					free_memory = true;
					APar_StandardTime(purd_time);
				} else {
					purd_time = optarg;
				}
			} else {
				purd_time = optarg;
			}
			
			AtomicInfo* globalIDData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.purd.data", optarg, AtomFlags_Data_Text);
			APar_Unified_atom_Put(globalIDData_atom, purd_time, UTF8_iTunesStyle_256glyphLimited, 0, 0);
			if (free_memory) {
				free(purd_time);
				purd_time = NULL;
			}
			break;
		}
		
		case Meta_PlayGapless : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "gapless playback") ) {
				break;
			}
			
			if (strncmp(optarg, "false", 5) == 0 || strlen(optarg) == 0) {
				APar_RemoveAtom("moov.udta.meta.ilst.pgap.data", VERSIONED_ATOM, 0);
			} else {
				//gapless playback: [0, 0, 0, 0,   boolean_value]; BUT that first uint32_t is already accounted for in APar_MetaData_atom_Init
				AtomicInfo* gaplessData_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.pgap.data", optarg, AtomFlags_Data_UInt);
				APar_Unified_atom_Put(gaplessData_atom, NULL, UTF8_iTunesStyle_256glyphLimited, 1, 8);
			}
			break;
		}
		
		case Meta_SortOrder : {
			AtomicInfo* sortOrder_atom = NULL;
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "sort order tags") ) {
				break;
			}
			
			if (argv[optind] == NULL) {
				fprintf(stdout, "AP warning, skipping setting the sort order %s tag\n", optarg);
				break;
			}
			
			if ( memcmp(optarg, "name", 5) == 0 ) {
				sortOrder_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.sonm.data", argv[optind], AtomFlags_Data_Text);			
			} else if ( memcmp(optarg, "artist", 7) == 0 ) {
				sortOrder_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.soar.data", argv[optind], AtomFlags_Data_Text);
			} else if ( memcmp(optarg, "albumartist", 12) == 0 ) {
				sortOrder_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.soaa.data", argv[optind], AtomFlags_Data_Text);
			} else if ( memcmp(optarg, "album", 6) == 0 ) {
				sortOrder_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.soal.data", argv[optind], AtomFlags_Data_Text);
			} else if ( memcmp(optarg, "composer", 9) == 0 ) {
				sortOrder_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.soco.data", argv[optind], AtomFlags_Data_Text);
			} else if ( memcmp(optarg, "show", 5) == 0 ) {
				sortOrder_atom = APar_MetaData_atom_Init("moov.udta.meta.ilst.sosn.data", argv[optind], AtomFlags_Data_Text);
			}
			APar_Unified_atom_Put(sortOrder_atom, argv[optind], UTF8_iTunesStyle_256glyphLimited, 0, 0);
			
			break;
		}
		
		//uuid atoms
		
		case Meta_StandardDate : {
			APar_ScanAtoms(ISObasemediafile);
			char* formed_time = (char *)malloc(sizeof(char)*110);
			if (argv[optind]) {
				if (strlen(argv[optind]) > 0) {
					APar_StandardTime(formed_time);
				}
			}
			
			AtomicInfo* tdtgUUID = APar_uuid_atom_Init("moov.udta.meta.uuid=%s", "tdtg", AtomFlags_Data_Text, formed_time, false);
			APar_Unified_atom_Put(tdtgUUID, formed_time, UTF8_iTunesStyle_Unlimited, 0, 0);
			free(formed_time);
			break;
		}
		
		case Meta_URL : {
			APar_ScanAtoms(ISObasemediafile);
			AtomicInfo* urlUUID = APar_uuid_atom_Init("moov.udta.meta.uuid=%s", "©url", AtomFlags_Data_Text, optarg, false);
			APar_Unified_atom_Put(urlUUID, optarg, UTF8_iTunesStyle_Unlimited, 0, 0);
			break;
		}
		
		case Meta_Information : {
			APar_ScanAtoms(ISObasemediafile);
			AtomicInfo* infoUUID = APar_uuid_atom_Init("moov.udta.meta.uuid=%s", "©inf", AtomFlags_Data_Text, optarg, false);
			APar_Unified_atom_Put(infoUUID, optarg, UTF8_iTunesStyle_Unlimited, 0, 0);
			break;
		}

		case Meta_uuid : {
			APar_ScanAtoms(ISObasemediafile);
			uint32_t uuid_dataType = 0;
			uint32_t desc_len = 0;
			uint8_t mime_len = 0;
			char* uuid_file_path = NULL;
			char* uuid_file_description = NULL;
			char* uuid_file_extn = NULL;
			char* uuid_file_mimetype = NULL;
//			char* uuid_file_filename = NULL;
			
			//a uuid in AP is a version 5 uuid created by getting a sha1 hash of a string (the atom name) in a namespace ('AP.sf.net'). This is guaranteed to be
			//reproducible, so later it can be verified that this uuid (which could come from anywhere), is in fact made by AtomicParsley. This is achieved by
			//storing the atom name string right after the uuid, and is read back later and a new uuid is created to see if it matches the discovered uuid. If
			//they match, it will print out or extract to a file; if not, only its name will be displayed in the tree.
			
			// --meta-uuid "©foo" 1 'http://www.url.org'  --meta-uuid "pdf1" file /some/path/pic.pdf description="My Booty, Your Booty, Djbouti"
			
			if ( memcmp(argv[optind], "text", 5) == 0  || memcmp(argv[optind], "1", 2) == 0 ) uuid_dataType = AtomFlags_Data_Text;
			if ( memcmp(argv[optind], "file", 5) == 0 ) {
				uuid_dataType = AtomFlags_Data_uuid_binary;
				if (optind+1 < argc) {
					if (true) { //TODO: test the file to see if it exists
						uuid_file_path = argv[optind + 1];
						//get the file extension/suffix of the file to embed
						uuid_file_extn = strrchr(uuid_file_path, '.'); //'.' inclusive; say goodbye to AP-0.8.8.tar.bz2
						
//#ifdef WIN32
//#define path_delim '\'
//#else
//#define path_delim '/'
//#endif
//						uuid_file_filename = strrchr(uuid_file_path, path_delim)+1; //includes whatever extensions
					}
					if (uuid_file_extn == NULL) {
						fprintf(stdout, "AP warning: embedding a file onto a uuid atom requires a file extension. Skipping.\n");
						continue;
					}
					//copy a pointer to description
					int more_optional_args = 2;
					while (optind + more_optional_args < argc) {
						if ( memcmp(argv[optind + more_optional_args], "description=", 12) == 0 && argv[optind + more_optional_args][12]) {
							uuid_file_description = argv[optind + more_optional_args] + 12;
							desc_len = strlen(uuid_file_description)+1; //+1 for the trailing 1 byte NULL terminator
						}
						if ( memcmp(argv[optind + more_optional_args], "mime-type=", 10) == 0 && argv[optind + more_optional_args][10]) {
							uuid_file_mimetype = argv[optind + more_optional_args] + 10;
							mime_len = strlen(uuid_file_mimetype)+1; //+1 for the trailing 1 byte NULL terminator
						}
						if (memcmp(argv[optind+more_optional_args], "--", 2) == 0) {
							break; //we've hit another cli argument
						}
						more_optional_args++;
					}
				}
			}
			
			AtomicInfo* genericUUID = APar_uuid_atom_Init("moov.udta.meta.uuid=%s", optarg, uuid_dataType, argv[optind +1], true);
			
			if (uuid_dataType == AtomFlags_Data_uuid_binary && genericUUID != NULL) {
				TestFileExistence(uuid_file_path, true);
				
//format for a uuid atom set by AP:
//4 bytes     - atom length as uin32_t
//4 bytes     - atom name as iso 8859-1 atom name as a 4byte string set to 'uuid'
//16 bytes    - the uuid; here a version 5 sha1-based hash derived from a name in a namespace of 'AtomicParsley.sf.net'
//4 bytes     - the name of the desired atom to create a uuid for (this "name" of the uuid is the only cli accessible means of crafting the uuid)
//4 bytes     - atom version & flags (currently 1 for 'text' or '88' for binary attachment)
//4 bytes     - NULL space
/////////////text or 1 for version/flags
		//X bytes - utf8 string, no null termination
/////////////binary attachment or 88 for version/flags
		//4 bytes - length of utf8 string describing the attachment
		//X bytes - utf8 string describing the attachment, null terminated
		//1 byte  - length of the file suffix (including the period) of the originating file
		//X bytes - utf8 string of the file suffix, null terminated
		//1 byte  - length of the MIME-type
		//X bytes - utf8 string holding the MIME-type, null terminated
		//4 bytes - length of the attached binary data/file length
		//X bytes - binary data
				
				uint32_t extn_len = strlen(uuid_file_extn)+1; //+1 for the trailing 1 byte NULL terminator
				uint32_t file_len = (uint32_t)findFileSize(uuid_file_path);
				
				APar_MetaData_atom_QuickInit(genericUUID->AtomicNumber, uuid_dataType, 20, extn_len + desc_len + file_len + 100);
				genericUUID->AtomicClassification = EXTENDED_ATOM; //it gets reset in QuickInit Above; force its proper setting
				
				if (uuid_file_description == NULL || desc_len == 0) {
					APar_Unified_atom_Put(genericUUID, "[none]", UTF8_3GP_Style, 7, 32); //sets 4bytes desc_len, then 7bytes description (forced to "[none]"=6 + 1 byte NULL =7)
				} else {
					APar_Unified_atom_Put(genericUUID, uuid_file_description, UTF8_3GP_Style, desc_len, 32); //sets 4bytes desc_len, then Xbytes description (in that order)
				}
				
				APar_Unified_atom_Put(genericUUID, uuid_file_extn, UTF8_3GP_Style, extn_len, 8); //sets 1bytes desc_len, then Xbytes file extension string (in that order)
				
				if (uuid_file_mimetype == NULL || mime_len == 0) {
					APar_Unified_atom_Put(genericUUID, "none", UTF8_3GP_Style, 5, 8); //sets 4bytes desc_len, then 5bytes description (forced to "none" + 1byte null)
				} else {
					APar_Unified_atom_Put(genericUUID, uuid_file_mimetype, UTF8_3GP_Style, mime_len, 8); //sets 1 byte mime len, then Xbytes mime type
				}
				
				FILE* uuid_binfile = APar_OpenFile(uuid_file_path, "rb");
				APar_Unified_atom_Put(genericUUID, NULL, UTF8_3GP_Style, file_len, 32);
				//store the data directly on the atom in AtomicData
				uint32_t bin_bytes_read = APar_ReadFile(genericUUID->AtomicData + (genericUUID->AtomicLength - 32), uuid_binfile, file_len);
				genericUUID->AtomicLength += bin_bytes_read;
				fclose(uuid_binfile);
				
			} else { //text type
				APar_Unified_atom_Put(genericUUID, argv[optind +1], UTF8_iTunesStyle_Unlimited, 0, 0);
			}

			break;
		}
		
		case Opt_Extract_all_uuids : {
			APar_ScanAtoms(ISObasemediafile);
			char* output_path = NULL;
			if (optind + 1 == argc) {
				output_path = argv[optind];
			}

			APar_OpenISOBaseMediaFile(ISObasemediafile, true);
			APar_Print_APuuid_atoms(ISObasemediafile, output_path, EXTRACT_ALL_UUID_BINARYS);
			APar_OpenISOBaseMediaFile(ISObasemediafile, false);
			
			exit(0);//never gets here
			break;
		}
		
		case Opt_Extract_a_uuid : {
			APar_ScanAtoms(ISObasemediafile);
			
			char* uuid_path = (char*)calloc(1, sizeof(char)*256+1);
			char* uuid_binary_str = (char*)calloc(1, sizeof(char)*20+1);
			char uuid_4char_name[16]; memset(uuid_4char_name, 0, 16);
			AtomicInfo* extractionAtom = NULL;
			
			UTF8Toisolat1((unsigned char*)&uuid_4char_name, 4, (unsigned char*)optarg, strlen(optarg) );
			APar_generate_uuid_from_atomname(uuid_4char_name, uuid_binary_str);
			
			//this will only append (and knock off) %s (anything) at the end of a string
			uint16_t path_len = strlen("moov.udta.meta.uuid=%s");
			memcpy(uuid_path, "moov.udta.meta.uuid=%s", path_len-2);
			memcpy(uuid_path + (path_len-2), uuid_binary_str, 16);
			
			extractionAtom = APar_FindAtom(uuid_path, false, EXTENDED_ATOM, 0, true);
			if (extractionAtom != NULL) {
				APar_OpenISOBaseMediaFile(ISObasemediafile, true);
				APar_Extract_uuid_binary_file(extractionAtom, ISObasemediafile, NULL);
				APar_OpenISOBaseMediaFile(ISObasemediafile, false);
			}
			
			free(uuid_path); uuid_path = NULL;
			free(uuid_binary_str); uuid_binary_str = NULL;
			exit(0);
			break; //never gets here
		}
		
		case Opt_Ipod_AVC_uuid : {
			if (deep_atom_scan == true) {
				if (memcmp(optarg, "1200", 3) != 0) {
					fprintf(stdout, "the ipod-uuid has a single preset of '1200' which is required\n"); //1200 might not be the only max macroblock setting down the pike
					break;
				}
				uint8_t total_tracks = 0;
				uint8_t a_track = 0;//unused
				char atom_path[100];
				AtomicInfo* video_desc_atom = NULL;
				AtomicInfo* ipod_uuid = NULL;
				
				memset(atom_path, 0, 100);
	
				APar_FindAtomInTrack(total_tracks, a_track, NULL); //With track_num set to 0, it will return the total trak atom into total_tracks here.
				
				while (a_track < total_tracks) {
					a_track++;
					sprintf(atom_path, "moov.trak[%hhu].mdia.minf.stbl.stsd.avc1", a_track);
					video_desc_atom = APar_FindAtom(atom_path, false, VERSIONED_ATOM, 0, false);
					
					if (video_desc_atom != NULL) {
						uint16_t mb_t = APar_TestVideoDescription(video_desc_atom, APar_OpenFile(ISObasemediafile, "rb"));
						if (mb_t > 0 && mb_t <= 1200) {
							sprintf(atom_path, "moov.trak[%hhu].mdia.minf.stbl.stsd.avc1.uuid=", a_track);
							uint8_t uuid_baselen = (uint8_t)strlen(atom_path);
							APar_uuid_scanf(atom_path + uuid_baselen, "6b6840f2-5f24-4fc5-ba39-a51bcf0323f3");
							APar_endian_uuid_bin_str_conversion(atom_path + uuid_baselen);
							APar_Generate_iPod_uuid(atom_path);
						}
					}
				}
				
			} else {
				fprintf(stdout, "the --DeepScan option is required for this operation. Skipping\n");
			}
			break;
		}
		
		case Manual_atom_removal : {
			APar_ScanAtoms(ISObasemediafile);
			
			char* compliant_name = (char*)malloc(sizeof(char)* strlen(optarg) +1);
			memset(compliant_name, 0, strlen(optarg) +1);
			UTF8Toisolat1((unsigned char*)compliant_name, strlen(optarg), (unsigned char*)optarg, strlen(optarg) );
			
			if (strstr(optarg, "uuid=") != NULL) {
				uint16_t uuid_path_pos = 0;
				uint16_t uuid_path_len = strlen(optarg);
				while (compliant_name+uuid_path_pos < compliant_name+uuid_path_len) {
					if (memcmp(compliant_name+uuid_path_pos, "uuid=", 5) == 0) {
						uuid_path_pos+=5;
						break;
					}
					uuid_path_pos++;
				}
				if (strlen(compliant_name+uuid_path_pos) > 4) { //if =4 then it would be the deprecated form (or it should be, if not it just won't find anything; no harm done)
					uint8_t uuid_len = APar_uuid_scanf(compliant_name+uuid_path_pos, optarg+uuid_path_pos);
					compliant_name[uuid_path_pos+uuid_len] = 0;
				}
				APar_RemoveAtom(compliant_name, EXTENDED_ATOM, 0);
				
			} else if (memcmp(compliant_name + (strlen(compliant_name) - 4), "data", 4) == 0) {
				APar_RemoveAtom(compliant_name, VERSIONED_ATOM, 0);
				
			} else {
				size_t string_len = strlen(compliant_name);
				//reverseDNS atom path
				if (strstr(optarg, ":[") != NULL && memcmp(compliant_name + string_len-1, "]", 1) == 0 ) {
					APar_RemoveAtom(compliant_name, VERSIONED_ATOM, 0);
				
				//packed language asset
				} else if (memcmp(compliant_name + string_len - 9, ":lang=", 6) == 0 ) {
					uint16_t packed_lang = PackLanguage(compliant_name + string_len - 3, 0);
					memset(compliant_name + string_len - 9, 0, 1);
					APar_RemoveAtom(compliant_name, PACKED_LANG_ATOM, packed_lang);
					
				} else {
					APar_RemoveAtom(compliant_name, UNKNOWN_ATOM, 0);
				}
			}
			free(compliant_name);
			compliant_name = NULL;
			break;
		}
		
		//3gp tags
		
		/*
		First, scan the file to get at atom tree (only happens once). Then take the cli args and look for optional arguments. All arguments begin with -- or -; other args
		are optional and are determined by directly testing arguments. Optional arguments common to the 3gp asset group (language, unicode, track/movie userdata) are
		extracted in find_optional_args. Setting assets in all tracks requires getting the number of tracks. Loop through either once (for movie & single track) or as
		many tracks there are for all tracks. Each pass through the loop, set the individual pieces of metadata.
		*/
		case _3GP_Title : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style >= THIRD_GEN_PARTNER && metadata_style < MOTIONJPEG2000, 2, "title") ) {
				break;
			}
			bool set_UTF16_text = false;
			uint16_t packed_lang = 0;
			uint8_t userdata_area = MOVIE_LEVEL_ATOM;
			uint8_t selected_track = 0;
			uint8_t a_track = 0;//unused
			uint8_t asset_iterations = 0;

			find_optional_args(argv, optind, packed_lang, set_UTF16_text, userdata_area, selected_track, 3);
			
			if (userdata_area == MOVIE_LEVEL_ATOM || userdata_area == SINGLE_TRACK_ATOM) {
				asset_iterations = 1;
			} else if (userdata_area == ALL_TRACKS_ATOM) {
				APar_FindAtomInTrack(asset_iterations, a_track, NULL); //With asset_iterations set to 0, it will return the total trak atom into total_tracks here.
				if (asset_iterations == 1) selected_track = 1; //otherwise, APar_UserData_atom_Init will shift to non-existing track 0
			}
			
			for (uint8_t i_asset = 1; i_asset <= asset_iterations; i_asset++) {
				AtomicInfo* title_asset = APar_UserData_atom_Init("titl", optarg, userdata_area, asset_iterations == 1 ? selected_track : i_asset, packed_lang);
				APar_Unified_atom_Put(title_asset, optarg, (set_UTF16_text ? UTF16_3GP_Style : UTF8_3GP_Style), (uint32_t)packed_lang, 16);
			}
			break;
		}
		
		case _3GP_Author : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style >= THIRD_GEN_PARTNER && metadata_style < MOTIONJPEG2000, 2, "author") ) {
				break;
			}
			bool set_UTF16_text = false;
			uint16_t packed_lang = 0;
			uint8_t userdata_area = MOVIE_LEVEL_ATOM;
			uint8_t selected_track = 0;
			uint8_t a_track = 0;//unused
			uint8_t asset_iterations = 0;

			find_optional_args(argv, optind, packed_lang, set_UTF16_text, userdata_area, selected_track, 3);
			
			if (userdata_area == MOVIE_LEVEL_ATOM || userdata_area == SINGLE_TRACK_ATOM) {
				asset_iterations = 1;
			} else if (userdata_area == ALL_TRACKS_ATOM) {
				APar_FindAtomInTrack(asset_iterations, a_track, NULL); //With asset_iterations set to 0, it will return the total trak atom into total_tracks here.
				if (asset_iterations == 1) selected_track = 1;
			}
			
			for (uint8_t i_asset = 1; i_asset <= asset_iterations; i_asset++) {
				AtomicInfo* author_asset = APar_UserData_atom_Init("auth", optarg, userdata_area, asset_iterations == 1 ? selected_track : i_asset, packed_lang);
				APar_Unified_atom_Put(author_asset, optarg, (set_UTF16_text ? UTF16_3GP_Style : UTF8_3GP_Style), (uint32_t)packed_lang, 16);
			}
			break;
		}
		
		case _3GP_Performer : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style >= THIRD_GEN_PARTNER && metadata_style < MOTIONJPEG2000, 2, "performer") ) {
				break;
			}
			bool set_UTF16_text = false;
			uint16_t packed_lang = 0;
			uint8_t userdata_area = MOVIE_LEVEL_ATOM;
			uint8_t selected_track = 0;
			uint8_t a_track = 0;//unused
			uint8_t asset_iterations = 0;

			find_optional_args(argv, optind, packed_lang, set_UTF16_text, userdata_area, selected_track, 3);
			
			if (userdata_area == MOVIE_LEVEL_ATOM || userdata_area == SINGLE_TRACK_ATOM) {
				asset_iterations = 1;
			} else if (userdata_area == ALL_TRACKS_ATOM) {
				APar_FindAtomInTrack(asset_iterations, a_track, NULL); //With asset_iterations set to 0, it will return the total trak atom into total_tracks here.
				if (asset_iterations == 1) selected_track = 1;
			}
			
			for (uint8_t i_asset = 1; i_asset <= asset_iterations; i_asset++) {
				AtomicInfo* performer_asset = APar_UserData_atom_Init("perf", optarg, userdata_area, asset_iterations == 1 ? selected_track : i_asset, packed_lang);
				APar_Unified_atom_Put(performer_asset, optarg, (set_UTF16_text ? UTF16_3GP_Style : UTF8_3GP_Style), (uint32_t)packed_lang, 16);
			}
			break;
		}
		
		case _3GP_Genre : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style >= THIRD_GEN_PARTNER && metadata_style < MOTIONJPEG2000, 2, "genre") ) {
				break;
			}
			bool set_UTF16_text = false;
			uint16_t packed_lang = 0;
			uint8_t userdata_area = MOVIE_LEVEL_ATOM;
			uint8_t selected_track = 0;
			uint8_t a_track = 0;//unused
			uint8_t asset_iterations = 0;

			find_optional_args(argv, optind, packed_lang, set_UTF16_text, userdata_area, selected_track, 3);
			
			if (userdata_area == MOVIE_LEVEL_ATOM || userdata_area == SINGLE_TRACK_ATOM) {
				asset_iterations = 1;
			} else if (userdata_area == ALL_TRACKS_ATOM) {
				APar_FindAtomInTrack(asset_iterations, a_track, NULL); //With asset_iterations set to 0, it will return the total trak atom into total_tracks here.
				if (asset_iterations == 1) selected_track = 1;
			}
			
			for (uint8_t i_asset = 1; i_asset <= asset_iterations; i_asset++) {
				AtomicInfo* genre_asset = APar_UserData_atom_Init("gnre", optarg, userdata_area, asset_iterations == 1 ? selected_track : i_asset, packed_lang);
				APar_Unified_atom_Put(genre_asset, optarg, (set_UTF16_text ? UTF16_3GP_Style : UTF8_3GP_Style), (uint32_t)packed_lang, 16);
			}
			break;
		}
		
		case _3GP_Description : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style >= THIRD_GEN_PARTNER && metadata_style < MOTIONJPEG2000, 2, "description") ) {
				break;
			}
			bool set_UTF16_text = false;
			uint16_t packed_lang = 0;
			uint8_t userdata_area = MOVIE_LEVEL_ATOM;
			uint8_t selected_track = 0;
			uint8_t a_track = 0;//unused
			uint8_t asset_iterations = 0;

			find_optional_args(argv, optind, packed_lang, set_UTF16_text, userdata_area, selected_track, 3);
			
			if (userdata_area == MOVIE_LEVEL_ATOM || userdata_area == SINGLE_TRACK_ATOM) {
				asset_iterations = 1;
			} else if (userdata_area == ALL_TRACKS_ATOM) {
				APar_FindAtomInTrack(asset_iterations, a_track, NULL); //With asset_iterations set to 0, it will return the total trak atom into total_tracks here.
				if (asset_iterations == 1) selected_track = 1;
			}
			
			for (uint8_t i_asset = 1; i_asset <= asset_iterations; i_asset++) {
				AtomicInfo* description_asset = APar_UserData_atom_Init("dscp", optarg, userdata_area, asset_iterations == 1 ? selected_track : i_asset, packed_lang);
				APar_Unified_atom_Put(description_asset, optarg, (set_UTF16_text ? UTF16_3GP_Style : UTF8_3GP_Style), (uint32_t)packed_lang, 16);
			}
			break;
		}
		
		case ISO_Copyright:       //ISO copyright atom common to all files that are derivatives of the base media file format, identical to....
		case _3GP_Copyright : {   //the 3gp copyright asset; this gets a test for major branding (but only with the cli arg --3gp-copyright).
			APar_ScanAtoms(ISObasemediafile);
			
			if (c == _3GP_Copyright) {
				if ( !APar_assert(metadata_style >= THIRD_GEN_PARTNER && metadata_style < MOTIONJPEG2000, 2, "copyright") ) {
					break;
				}
			}
			
			bool set_UTF16_text = false;
			uint16_t packed_lang = 0;
			uint8_t userdata_area = MOVIE_LEVEL_ATOM;
			uint8_t selected_track = 0;
			uint8_t a_track = 0;//unused
			uint8_t asset_iterations = 0;

			find_optional_args(argv, optind, packed_lang, set_UTF16_text, userdata_area, selected_track, 3);
			
			if (userdata_area == MOVIE_LEVEL_ATOM || userdata_area == SINGLE_TRACK_ATOM) {
				asset_iterations = 1;
			} else if (userdata_area == ALL_TRACKS_ATOM) {
				APar_FindAtomInTrack(asset_iterations, a_track, NULL); //With asset_iterations set to 0, it will return the total trak atom into total_tracks here.
				if (asset_iterations == 1) selected_track = 1;
			}
			
			for (uint8_t i_asset = 1; i_asset <= asset_iterations; i_asset++) {
				AtomicInfo* copyright_notice = APar_UserData_atom_Init("cprt", optarg, userdata_area, asset_iterations == 1 ? selected_track : i_asset, packed_lang);
				APar_Unified_atom_Put(copyright_notice, optarg, (set_UTF16_text ? UTF16_3GP_Style : UTF8_3GP_Style), (uint32_t)packed_lang, 16);
			}
			break;
		}
		
		case _3GP_Album : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style >= THIRD_GEN_PARTNER && metadata_style < MOTIONJPEG2000, 2, "album") ) {
				break;
			}
			bool set_UTF16_text = false;
			uint16_t packed_lang = 0;
			uint8_t userdata_area = MOVIE_LEVEL_ATOM;
			uint8_t selected_track = 0;
			uint8_t a_track = 0;//unused
			uint8_t asset_iterations = 0;
			uint8_t tracknum = 0;
			
			find_optional_args(argv, optind, packed_lang, set_UTF16_text, userdata_area, selected_track, 4);
			
			//cygle through the remaining independant arguments (before the next --cli_flag) and figure out if any are useful to us; already have lang & utf16
			for (int i= 0; i <= 4; i++) { //3 possible arguments for this tag (the first - which doesn't count - is the data for the tag itself)
				if ( argv[optind + i] && optind + i <= total_args) {
					if ( memcmp(argv[optind + i], "trknum=", 7) == 0 ) {
						char* track_num = argv[optind + i];
						strsep(&track_num,"=");
						sscanf(track_num, "%hhu", &tracknum);
					}
					if (memcmp(argv[optind + i], "-", 1) == 0) break;
				}
			}

			if (userdata_area == MOVIE_LEVEL_ATOM || userdata_area == SINGLE_TRACK_ATOM) {
				asset_iterations = 1;
			} else if (userdata_area == ALL_TRACKS_ATOM) {
				APar_FindAtomInTrack(asset_iterations, a_track, NULL); //With asset_iterations set to 0, it will return the total trak atom into total_tracks here.
				if (asset_iterations == 1) selected_track = 1;
			}
			
			for (uint8_t i_asset = 1; i_asset <= asset_iterations; i_asset++) {
				AtomicInfo* album_asset = APar_UserData_atom_Init("albm", optarg, userdata_area, asset_iterations == 1 ? selected_track : i_asset, packed_lang);
				APar_Unified_atom_Put(album_asset, optarg, (set_UTF16_text ? UTF16_3GP_Style : UTF8_3GP_Style), (uint32_t)packed_lang, 16);
				if (tracknum != 0) {
					APar_Unified_atom_Put(album_asset, NULL, UTF8_3GP_Style, (uint32_t)tracknum, 8);
				}
			}			
			break;
		}
		
		case _3GP_Year : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style >= THIRD_GEN_PARTNER && metadata_style < MOTIONJPEG2000, 2, "year") ) {
				break;
			}
			uint8_t userdata_area = MOVIE_LEVEL_ATOM;
			uint8_t selected_track = 0;
			uint8_t a_track = 0;//unused
			uint8_t asset_iterations = 0;
			uint16_t year_tag = 0;

			if ( argv[optind] && optind <= total_args) {
				if ( memcmp(argv[optind], "movie", 6) == 0 ) {
					userdata_area = MOVIE_LEVEL_ATOM;					}
				if ( memcmp(argv[optind], "track=", 6) == 0 ) {
					char* trak_idx = argv[optind];
					strsep(&trak_idx, "=");
					sscanf(trak_idx, "%hhu", &selected_track);
					userdata_area = SINGLE_TRACK_ATOM;	
				} else if ( memcmp(argv[optind], "track", 6) == 0 ) {
					userdata_area = ALL_TRACKS_ATOM;	
				}
			}
			
			sscanf(optarg, "%hu", &year_tag);
			
			if (userdata_area == MOVIE_LEVEL_ATOM || userdata_area == SINGLE_TRACK_ATOM) {
				asset_iterations = 1;
			} else if (userdata_area == ALL_TRACKS_ATOM) {
				APar_FindAtomInTrack(asset_iterations, a_track, NULL); //With asset_iterations set to 0, it will return the total trak atom into total_tracks here.
				if (asset_iterations == 1) selected_track = 1;
			}
			
			for (uint8_t i_asset = 1; i_asset <= asset_iterations; i_asset++) {
				AtomicInfo* recordingyear_asset = APar_UserData_atom_Init("yrrc", optarg, userdata_area, asset_iterations == 1 ? selected_track : i_asset, 0);
				APar_Unified_atom_Put(recordingyear_asset, NULL, UTF8_3GP_Style, (uint32_t)year_tag, 16);
			}
			break;	
		}
		
		case _3GP_Rating : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style >= THIRD_GEN_PARTNER && metadata_style < MOTIONJPEG2000, 2, "rating") ) {
				break;
			}
			char rating_entity[5] = { 'N', 'O', 'N', 'E', 0 }; //'NONE' - thats what it will be if not provided
			char rating_criteria[5] = { 'N', 'O', 'N', 'E', 0 };
			bool set_UTF16_text = false;
			uint16_t packed_lang = 0;
			uint8_t userdata_area = MOVIE_LEVEL_ATOM;
			uint8_t selected_track = 0;
			uint8_t a_track = 0;//unused
			uint8_t asset_iterations = 0;

			find_optional_args(argv, optind, packed_lang, set_UTF16_text, userdata_area, selected_track, 5);
						
			for (int i= 0; i < 5; i++) { //3 possible arguments for this tag (the first - which doesn't count - is the data for the tag itself)
				if ( argv[optind + i] && optind + i <= total_args) {
					if ( memcmp(argv[optind + i], "entity=", 7) == 0 ) {
						char* entity = argv[optind + i];
						strsep(&entity,"=");
						memcpy(&rating_entity, entity, 4);
					}
					if ( memcmp(argv[optind + i], "criteria=", 9) == 0 ) {
						char* criteria = argv[optind + i];
						strsep(&criteria,"=");
						memcpy(&rating_criteria, criteria, 4);
					}
					if (memcmp(argv[optind + i], "-", 1) == 0) break; //we've hit another cli argument
				}
			}
			
			if (userdata_area == MOVIE_LEVEL_ATOM || userdata_area == SINGLE_TRACK_ATOM) {
				asset_iterations = 1;
			} else if (userdata_area == ALL_TRACKS_ATOM) {
				APar_FindAtomInTrack(asset_iterations, a_track, NULL); //With asset_iterations set to 0, it will return the total trak atom into total_tracks here.
				if (asset_iterations == 1) selected_track = 1;
			}
			
			for (uint8_t i_asset = 1; i_asset <= asset_iterations; i_asset++) {
				AtomicInfo* rating_asset = APar_UserData_atom_Init("rtng", optarg, userdata_area, asset_iterations == 1 ? selected_track : i_asset, packed_lang);
			
				APar_Unified_atom_Put(rating_asset, NULL, UTF8_3GP_Style, UInt32FromBigEndian(rating_entity), 32);
				APar_Unified_atom_Put(rating_asset, NULL, UTF8_3GP_Style, UInt32FromBigEndian(rating_criteria), 32);
				APar_Unified_atom_Put(rating_asset, optarg, (set_UTF16_text ? UTF16_3GP_Style : UTF8_3GP_Style), (uint32_t)packed_lang, 16);
			}
			break;	
		}
		
		case _3GP_Classification : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style >= THIRD_GEN_PARTNER && metadata_style < MOTIONJPEG2000, 2, "classification") ) {
				break;
			}
			char classification_entity[5] = { 'N', 'O', 'N', 'E', 0 }; //'NONE' - thats what it will be if not provided
			uint16_t classification_index = 0;
			bool set_UTF16_text = false;
			uint16_t packed_lang = 0;
			uint8_t userdata_area = MOVIE_LEVEL_ATOM;
			uint8_t selected_track = 0;
			uint8_t a_track = 0;//unused
			uint8_t asset_iterations = 0;
			
			find_optional_args(argv, optind, packed_lang, set_UTF16_text, userdata_area, selected_track, 5);
			
			for (int i= 0; i < 4; i++) { //3 possible arguments for this tag (the first - which doesn't count - is the data for the tag itself)
				if ( argv[optind + i] && optind + i <= total_args) {
					if ( memcmp(argv[optind + i], "entity=", 7) == 0 ) {
						char* cls_entity = argv[optind + i];
						strsep(&cls_entity, "=");
						memcpy(&classification_entity, cls_entity, 4);
					}
					if ( memcmp(argv[optind + i], "index=", 6) == 0 ) {
						char* cls_idx = argv[optind + i];
						strsep(&cls_idx, "=");
						sscanf(cls_idx, "%hu", &classification_index);
					}
					if (memcmp(argv[optind + i], "-", 1) == 0) break; //we've hit another cli argument
				}
			}
			
			if (userdata_area == MOVIE_LEVEL_ATOM || userdata_area == SINGLE_TRACK_ATOM) {
				asset_iterations = 1;
			} else if (userdata_area == ALL_TRACKS_ATOM) {
				APar_FindAtomInTrack(asset_iterations, a_track, NULL); //With asset_iterations set to 0, it will return the total trak atom into total_tracks here.
				if (asset_iterations == 1) selected_track = 1;
			}
			
			for (uint8_t i_asset = 1; i_asset <= asset_iterations; i_asset++) {
				AtomicInfo* classification_asset = APar_UserData_atom_Init("clsf", optarg, userdata_area, asset_iterations == 1 ? selected_track : i_asset, packed_lang);
				
				APar_Unified_atom_Put(classification_asset, NULL, UTF8_3GP_Style, UInt32FromBigEndian(classification_entity), 32);
				APar_Unified_atom_Put(classification_asset, NULL, UTF8_3GP_Style, classification_index, 16);
				APar_Unified_atom_Put(classification_asset, optarg, (set_UTF16_text ? UTF16_3GP_Style : UTF8_3GP_Style), (uint32_t)packed_lang, 16);
			}
			break;	
		}
		
		case _3GP_Keyword : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style >= THIRD_GEN_PARTNER && metadata_style < MOTIONJPEG2000, 2, "keyword") ) {
				break;
			}
			bool set_UTF16_text = false;
			uint16_t packed_lang = 0;
			uint8_t userdata_area = MOVIE_LEVEL_ATOM;
			uint8_t selected_track = 0;
			uint8_t a_track = 0;//unused
			uint8_t asset_iterations = 0;
			char* formed_keyword_struct = NULL;

			find_optional_args(argv, optind, packed_lang, set_UTF16_text, userdata_area, selected_track, 4);
			
			if (userdata_area == MOVIE_LEVEL_ATOM || userdata_area == SINGLE_TRACK_ATOM) {
				asset_iterations = 1;
			} else if (userdata_area == ALL_TRACKS_ATOM) {
				APar_FindAtomInTrack(asset_iterations, a_track, NULL); //With asset_iterations set to 0, it will return the total trak atom into total_tracks here.
				if (asset_iterations == 1) selected_track = 1;
			}
			
			if (strrchr(optarg, '=') != NULL) { //must be in the format of:   keywords=foo1,foo2,foo3,foo4
				char* arg_keywords = optarg;
				char* keywords_globbed = strsep(&arg_keywords,"="); //separate out 'keyword='
				keywords_globbed = strsep(&arg_keywords,"="); //this is what we want to work on: just the keywords
				char* keyword_ptr = keywords_globbed;
				uint32_t keyword_strlen = strlen(keywords_globbed);
				uint8_t keyword_count = 0;
				uint32_t key_index = 0;
				
				if (keyword_strlen > 0) { //if there is anything past the = then it counts as a keyword
					keyword_count++;
				}
				
				while (true) { //count occurrences of comma here
					if (*keyword_ptr == ',') {
						keyword_count++;
					}
					keyword_ptr++;
					key_index++;
					if (keyword_strlen == key_index) {
						break;
					}
				}
				
				formed_keyword_struct = (char*)calloc(1, sizeof(char)* set_UTF16_text ? keyword_strlen * 4 : keyword_strlen * 2); // *4 should carry utf16's BOM & TERM
				uint32_t keyword_struct_bytes = APar_3GP_Keyword_atom_Format(keywords_globbed, keyword_count, set_UTF16_text, formed_keyword_struct);
				
				for (uint8_t i_asset = 1; i_asset <= asset_iterations; i_asset++) {
					AtomicInfo* keyword_asset = APar_UserData_atom_Init("kywd", keyword_strlen ? "temporary" : "", userdata_area, asset_iterations == 1 ? selected_track : i_asset, packed_lang); //just a "temporary" valid string to satisfy a test there
					if (keyword_strlen > 0) {
						APar_Unified_atom_Put(keyword_asset, NULL, UTF8_3GP_Style, (uint32_t)packed_lang, 16);
						APar_Unified_atom_Put(keyword_asset, NULL, UTF8_3GP_Style, keyword_count, 8);
						APar_atom_Binary_Put(keyword_asset, formed_keyword_struct, keyword_struct_bytes, 3);
					}
				}
				if (formed_keyword_struct != NULL) {
					free(formed_keyword_struct);
					formed_keyword_struct = NULL;
				}
			} else {
				for (uint8_t i_asset = 1; i_asset <= asset_iterations; i_asset++) {
					APar_UserData_atom_Init("kywd", "", userdata_area, asset_iterations == 1 ? selected_track : i_asset, packed_lang);
				}
			}
			break;	
		}
		
		case _3GP_Location : {
			APar_ScanAtoms(ISObasemediafile);
			if ( !APar_assert(metadata_style >= THIRD_GEN_PARTNER && metadata_style < MOTIONJPEG2000, 2, "location") ) {
				break;
			}
			bool set_UTF16_text = false;
			uint16_t packed_lang = 0;
			uint8_t userdata_area = MOVIE_LEVEL_ATOM;
			uint8_t selected_track = 0;
			uint8_t a_track = 0;//unused
			uint8_t asset_iterations = 0;
			double longitude = -73.98; //if you don't provide a place, you WILL be put right into Central Park. Welcome to New York City... now go away.
			double latitude = 40.77;
			double altitude = 4.3;
			uint8_t role = 0;
			char* astronomical_body = "Earth";
			char* additional_notes = "no notes";
			
			find_optional_args(argv, optind, packed_lang, set_UTF16_text, userdata_area, selected_track, 10);
			
			for (int i= 0; i <= 10; i++) { //9 possible arguments for this tag (the first - which doesn't count - is the data for the tag itself)
				if ( argv[optind + i] && optind + i <= total_args) {
					if ( memcmp(argv[optind + i], "longitude=", 10) == 0 ) {
						char* _long = argv[optind + i];
						strsep(&_long,"=");
						sscanf(_long, "%lf", &longitude);
						if (_long[strlen(_long)-1] == 'W') {
							longitude*=-1;
						}
					}
					if ( memcmp(argv[optind + i], "latitude=", 9) == 0 ) {
						char* _latt = argv[optind + i];
						strsep(&_latt,"=");
						sscanf(_latt, "%lf", &latitude);
						if (_latt[strlen(_latt)-1] == 'S') {
							latitude*=-1;
						}
					}
					if ( memcmp(argv[optind + i], "altitude=", 9) == 0 ) {
						char* _alti = argv[optind + i];
						strsep(&_alti,"=");
						sscanf(_alti, "%lf", &altitude);
						if (_alti[strlen(_alti)-1] == 'B') {
							altitude*=-1;
						}
					}
					if ( memcmp(argv[optind + i], "role=", 5) == 0 ) {
						char* _role = argv[optind + i];
						strsep(&_role,"=");
						if (strncmp(_role, "shooting location", 17) == 0 || strncmp(_role, "shooting", 8) == 0) {
							role = 0;
						} else if (strncmp(_role, "real location", 13) == 0 || strncmp(_role, "real", 4) == 0) {
							role = 1;
						} else if (strncmp(_role, "fictional location", 18) == 0 || strncmp(_role, "fictional", 9) == 0) {
							role = 2;
						}
					}
					if ( memcmp(argv[optind + i], "body=", 5) == 0 ) {
						char* _astrobody = argv[optind + i];
						strsep(&_astrobody,"=");
						astronomical_body = _astrobody;
					}
					if ( memcmp(argv[optind + i], "notes=", 6) == 0 ) {
						char* _add_notes = argv[optind + i];
						strsep(&_add_notes,"=");
						additional_notes = _add_notes;
					}
					if (memcmp(argv[optind + i], "-", 1) == 0) break; //we've hit another cli argument
				}
			}
			
			//fprintf(stdout, "long, lat, alt = %lf %lf %lf\n", longitude, latitude, altitude);
			if (userdata_area == MOVIE_LEVEL_ATOM || userdata_area == SINGLE_TRACK_ATOM) {
				asset_iterations = 1;
			} else if (userdata_area == ALL_TRACKS_ATOM) {
				APar_FindAtomInTrack(asset_iterations, a_track, NULL); //With asset_iterations set to 0, it will return the total trak atom into total_tracks here.
				if (asset_iterations == 1) selected_track = 1;
			}
			
			if (longitude < -180.0 || longitude > 180.0 || latitude < -90.0 || latitude > 90.0) {
				fprintf(stdout, "AtomicParsley warning: longitude or latitude was invalid; skipping setting location\n");
			} else {
				for (uint8_t i_asset = 1; i_asset <= asset_iterations; i_asset++) {
					//short location_3GP_atom = APar_UserData_atom_Init("moov.udta.loci", optarg, packed_lang);
					AtomicInfo* location_asset = APar_UserData_atom_Init("loci", optarg, userdata_area, asset_iterations == 1 ? selected_track : i_asset, packed_lang);
					APar_Unified_atom_Put(location_asset, optarg, (set_UTF16_text ? UTF16_3GP_Style : UTF8_3GP_Style), (uint32_t)packed_lang, 16);
					APar_Unified_atom_Put(location_asset, NULL, false, (uint32_t)role, 8);
					
					APar_Unified_atom_Put(location_asset, NULL, false, float_to_16x16bit_fixed_point(longitude), 32);
					APar_Unified_atom_Put(location_asset, NULL, false, float_to_16x16bit_fixed_point(latitude), 32);
					APar_Unified_atom_Put(location_asset, NULL, false, float_to_16x16bit_fixed_point(altitude), 32);
					APar_Unified_atom_Put(location_asset, astronomical_body, (set_UTF16_text ? UTF16_3GP_Style : UTF8_3GP_Style), 0, 0);
					APar_Unified_atom_Put(location_asset, additional_notes, (set_UTF16_text ? UTF16_3GP_Style : UTF8_3GP_Style), 0, 0);
				}
			}
			break;	
		}
		
		case Meta_ReverseDNS_Form : { //--rDNSatom "mv-ma" name=iTuneEXTC domain=com.apple.iTunes			
			char* reverseDNS_atomname = NULL;
			char* reverseDNS_atomdomain = NULL;
			uint32_t rdns_atom_flags = AtomFlags_Data_Text;
			
			APar_ScanAtoms(ISObasemediafile);
			
			if ( !APar_assert(metadata_style == ITUNES_STYLE, 1, "reverse DNS form") ) {
				break;
			}
			
			for (int i= 0; i <= 5-1; i++) {
				if ( argv[optind + i] && optind + i <= argc ) {
					if ( memcmp(argv[optind + i], "name=", 5) == 0 ) {
						reverseDNS_atomname = argv[optind + i]+5;
					} else if ( memcmp(argv[optind + i], "domain=", 7) == 0 ) {
						reverseDNS_atomdomain = argv[optind + i]+7;
					} else if ( memcmp(argv[optind + i], "datatype=", 9) == 0 ) {
						sscanf(argv[optind + i]+9, "%u", &rdns_atom_flags);
					}
					if (memcmp(argv[optind + i], "-", 1) == 0) {
						break; //we've hit another cli argument
					}
				}
			}
			
			if (reverseDNS_atomname == NULL) {
				fprintf(stdout, "AtomicParsley warning: no name for the reverseDNS form was found. Skipping.\n");
				
			} else if ((int)strlen(reverseDNS_atomname) != test_conforming_alpha_string(reverseDNS_atomname) ) {
				fprintf(stdout, "AtomicParsley warning: Some part of the reverseDNS atom name was non-conforming. Skipping.\n");
				
			} else if (reverseDNS_atomdomain == NULL) {
				fprintf(stdout, "AtomicParsley warning: no domain for the reverseDNS form was found. Skipping.\n");
				
			} else if (rdns_atom_flags != AtomFlags_Data_Text) {
				fprintf(stdout, "AtomicParsley warning: currently, only the strings are supported in reverseDNS atoms. Skipping.\n");
				
			} else {
				AtomicInfo* rDNS_data_atom = APar_reverseDNS_atom_Init(reverseDNS_atomname, optarg, &rdns_atom_flags, reverseDNS_atomdomain);
				APar_Unified_atom_Put(rDNS_data_atom, optarg, UTF8_iTunesStyle_Unlimited, 0, 0);
			}
			
			break;
		}
		
		case Meta_rDNS_rating : {
			char* media_rating = Expand_cli_mediastring(optarg);
			uint32_t rDNS_data_flags = AtomFlags_Data_Text;
			
			APar_ScanAtoms(ISObasemediafile);
			
			if (media_rating != NULL || strlen(optarg) == 0) {
				AtomicInfo* rDNS_rating_data_atom = APar_reverseDNS_atom_Init("iTunEXTC", media_rating, &rDNS_data_flags, "com.apple.iTunes");
				APar_Unified_atom_Put(rDNS_rating_data_atom, media_rating, UTF8_iTunesStyle_Unlimited, 0, 0);
			}
		
			break;
		}
		
		case Meta_ID3v2Tag : {
			char* target_frame_ID = NULL;
			uint16_t packed_lang = 0;
			uint8_t char_encoding = TE_UTF8; //utf8 is the default encoding
			char meta_container = 0-MOVIE_LEVEL_ATOM;
			bool multistring = false;
			APar_ScanAtoms(ISObasemediafile);
			
			//limit the files that can be tagged with meta.ID32 atoms. The file has to conform to the ISO BMFFv2 in order for a 'meta' atom.
			//This should exclude files branded as 3gp5 for example, except it doesn't always. The test is for a compatible brand (of a v2 ISO MBFF).
			//Quicktime writes some 3GPP files as 3gp5 with a compatible brand of mp42, so tagging works on these files. Not when you use timed text though.
			if ( !APar_assert(parsedAtoms[0].ancillary_data != 0 || (metadata_style >= THIRD_GEN_PARTNER_VER1_REL7 && metadata_style < MOTIONJPEG2000), 3, NULL) ) {
				break;
			}
			AdjunctArgs* id3args = (AdjunctArgs*)malloc(sizeof(AdjunctArgs));
			
			id3args->targetLang = NULL; //it will default later to "eng"
			id3args->descripArg = NULL;
			id3args->mimeArg = NULL;
			id3args->pictypeArg = NULL;
			id3args->ratingArg = NULL;
			id3args->dataArg = NULL;
			id3args->pictype_uint8 = 0;
			id3args->groupSymbol = 0;
			id3args->zlibCompressed = false;
			id3args->multistringtext = false;

			target_frame_ID = ConvertCLIFrameStr_TO_frameID(optarg);
			if (target_frame_ID == NULL) {
				target_frame_ID = optarg;
			}
			
			int frameType = FrameStr_TO_FrameType(target_frame_ID);
			if (frameType >= 0) {
				if (TestCLI_for_FrameParams(frameType, 0)) {
					id3args->descripArg = find_ID3_optarg(argv, optind, "desc=");
				}
				if (TestCLI_for_FrameParams(frameType, 1)) {
					id3args->mimeArg = find_ID3_optarg(argv, optind, "mimetype=");
				}
				if (TestCLI_for_FrameParams(frameType, 2)) {
					id3args->pictypeArg = find_ID3_optarg(argv, optind, "imagetype=");
				}
				if (TestCLI_for_FrameParams(frameType, 3)) {
					id3args->dataArg = find_ID3_optarg(argv, optind, "uniqueID=");
				}
				if (TestCLI_for_FrameParams(frameType, 4)) {
					id3args->filenameArg = find_ID3_optarg(argv, optind, "filename=");
				}
				if (TestCLI_for_FrameParams(frameType, 5)) {
					id3args->ratingArg = find_ID3_optarg(argv, optind, "rating=");
				}
				if (TestCLI_for_FrameParams(frameType, 6)) {
					id3args->dataArg = find_ID3_optarg(argv, optind, "counter=");
				}
				if (TestCLI_for_FrameParams(frameType, 7)) {
					id3args->dataArg = find_ID3_optarg(argv, optind, "data=");
				}
				if (TestCLI_for_FrameParams(frameType, 8)) {
					id3args->dataArg = find_ID3_optarg(argv, optind, "data=");
				}
				if (memcmp("1", find_ID3_optarg(argv, optind, "compressed"), 1) == 0) {
					id3args->zlibCompressed = true;
				}
				
				char* groupsymbol = find_ID3_optarg(argv, optind, "groupsymbol=");
				if (groupsymbol[0] == '0' && groupsymbol[1] == 'x') {
					sscanf(groupsymbol, "%hhX", &id3args->groupSymbol);
					if (id3args->groupSymbol < 0x80 || id3args->groupSymbol > 0xF0) id3args->groupSymbol = 0;
				}
			}
			
			scan_ID3_optargs(argv, optind, id3args->targetLang, packed_lang, char_encoding, &meta_container, multistring);
			if (id3args->targetLang == NULL) id3args->targetLang = "eng";
			
			APar_OpenISOBaseMediaFile(ISObasemediafile, true); //if not already scanned, the whole tag for *this* ID32 atom needs to be read from file
			AtomicInfo* id32_atom = APar_ID32_atom_Init(target_frame_ID, meta_container, id3args->targetLang, packed_lang);
			
			if (memcmp(argv[optind + 0], "extract", 7) == 0 && (memcmp(target_frame_ID, "APIC", 4) == 0 || memcmp(target_frame_ID, "GEOB", 4) == 0)) {
				if (id32_atom != NULL) {
					APar_Extract_ID3v2_file(id32_atom, target_frame_ID, ISObasemediafile, NULL, id3args);
					APar_OpenISOBaseMediaFile(ISObasemediafile, false);
				}
				exit(0);
			} 
			
			APar_OpenISOBaseMediaFile(ISObasemediafile, false);
			APar_ID3FrameAmmend(id32_atom, target_frame_ID, argv[optind + 0], id3args, char_encoding);
			
			free(id3args);
			id3args = NULL;
			
			break;
		}
		
		//utility functions
		
		case Metadata_Purge : {
			APar_ScanAtoms(ISObasemediafile);
			APar_RemoveAtom("moov.udta.meta.ilst", SIMPLE_ATOM, 0);
			
			break;
		}
		
		case UserData_Purge : {
			APar_ScanAtoms(ISObasemediafile);
			APar_RemoveAtom("moov.udta", SIMPLE_ATOM, 0);
			
			break;
		}
		
		case foobar_purge : {
			APar_ScanAtoms(ISObasemediafile);
			APar_RemoveAtom("moov.udta.tags", UNKNOWN_ATOM, 0);
			
			break;
		}
		
		case Opt_FreeFree : {
			APar_ScanAtoms(ISObasemediafile);
			int free_level = -1;
			if (argv[optind]) {
				sscanf(argv[optind], "%i", &free_level);
			}
			APar_freefree(free_level);
			
			break;
		}
				
		case OPT_OverWrite : {
			alter_original = true;
			break;
		}
		
		case Meta_dump : {
			APar_ScanAtoms(ISObasemediafile);
			APar_OpenISOBaseMediaFile(ISObasemediafile, true);
			APar_MetadataFileDump(ISObasemediafile);
			APar_OpenISOBaseMediaFile(ISObasemediafile, false);
			
			APar_FreeMemory();
			#if defined (_MSC_VER)
				for(int zz=0; zz < argc; zz++) {
					if (argv[zz] > 0) {
						free(argv[zz]);
						argv[zz] = NULL;
					}
				}
			#endif
			exit(0); //das right, this is a flag that doesn't get used with other flags.
		}
		
		case OPT_NoOptimize : {
			force_existing_hierarchy = true;
			break;
		}
		
		case OPT_OutputFile : {
			output_file = optarg;
			break;
		}
		
		} /* end switch */
	} /* end while */
	
	//after all the modifications are enacted on the tree in memory, THEN write out the changes
	if (modified_atoms) {
		APar_DetermineAtomLengths();
		APar_OpenISOBaseMediaFile(ISObasemediafile, true);
		APar_WriteFile(ISObasemediafile, output_file, alter_original);
		if (!alter_original) {
			//The file was opened orignally as read-only; when it came time to writeback into the original file, that FILE was closed, and a new one opened with write abilities, so to close a FILE that no longer exists would.... be retarded.
			APar_OpenISOBaseMediaFile(ISObasemediafile, false);
		}
	} else {
		if (ISObasemediafile != NULL && argc > 3 && !deep_atom_scan) {
			fprintf(stdout, "No changes.\n");
		}
	}
	APar_FreeMemory();
#if defined (_MSC_VER)
	for(int zz=0; zz < argc; zz++) {
		if (argv[zz] > 0) {
			free(argv[zz]);
			argv[zz] = NULL;
		}
	}
#endif
	return 0;
}
