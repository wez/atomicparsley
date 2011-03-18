#!/usr/bin/perl
#
# build --rDNSatom iTunMOVI data for AtomicParsley and write or print to file
#
# by HolyRoses
#
# example line from film "Extract"
# http://www.imdb.com/title/tt1225822/fullcredits
#
#./iTunMOVI.pl --cast "Jason Bateman" --cast "Mila Kunis,Kristen Wiig" --directors "Mike Judge" --producers "John Altschuler,Michael Flynn" --studio "Miramax Films" --copy_warning "FBI ANTI-PIRACY WARNING: UNAUTHORIZED COPYING IS PUNISHABLE UNDER FEDERAL LAW." --screenwriters "Mike Judge" --codirectors "Jasmine Alhambra,Maria Mantia" --print

use Getopt::Long;
#Getopt::Long::Configure ("bundling");

# default values for some items.
$ripper = "HolyRoses"; 
$copy_warning = "Help control the pet population. Have your pets spayed or neutered.";
$studio = "A $ripper Production";

GetOptions (
	'version' => \$version,
	'help' => \$help,
	'print'=> \$print,
	'write' => \$write,
	'file=s' => \$file,
	'copy_warning=s' => \$copy_warning,
	'studio=s' => \$studio,
	'cast|actors=s' => \@cast,
	'directors=s' => \@directors,
	'codirectors=s' => \@codirectors,
	'screenwriters=s' => \@screenwriters,
	'producers=s' => \@producers);

#print help
if ( $help ) {
	print << "EOF";

AtomicParsley iTunMOVI writer for Apple TV and iTunes.

The options cast|actors, directors, codirectors, screenwriters, producers can take multiple of the same type.

You can issue --cast "x person"  --cast "y person" or --cast "x person,y person,b person"
-------------------------------------------------------------------------------------------------------------
Atom options:
--copy_warning:		Add copy warning (displayed in iTunes summary page)
--studio:		Add film studio (displayed on Apple TV)
--cast|actors:		Add Actors (displayed on Apple TV and iTunes under long description)
--directors:		Add Directors (displayed on Apple TV and iTunes under long description)
--producers:		Add Producers (displayed on Apple TV and iTunes under long description)
--codirectors:		Add Co-Directors (displayed in iTunes under long description)
--screenwriters:	Add ScreenWriters (displayed in iTunes under long description)

Write and print options:
--print:		Display XML data to screen
--file:			MP4 file to write XML information to
--write:		Write XML data to MP4 file with AtomicParlsey

Misc options:
--version:		Displays version
--help:			Displays help
-------------------------------------------------------------------------------------------------------------
If there is more than 1 entry of cast, directors, producers, codirectors, screenwriters then in iTunes the verbage changes to plural. (PRODUCER -> PRODUCERS).

Apple TV display items:
Actors, Directors, Producers are displayed for Movies.
Actors are displayed for TV shows.
Studio is displayed on all videos (while video is playing press up arrow 2x, it is located under Title).

EOF
exit;
}

# version
$version_number = "1.1";

if ( $version ) {
	print "\n$0 v$version_number\n\n";
	print "Written by HolyRoses\n\n";
	exit;
}


# process arrays, join multiple --options of same type and join options seperated by commas
@cast = split(/,/,join(',',@cast));
@directors = split(/,/,join(',',@directors));
@producers = split(/,/,join(',',@producers));
@codirectors = split(/,/,join(',',@codirectors));
@screenwriters = split(/,/,join(',',@screenwriters));

# set header
$iTunMOVI_data = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
";

# set copy-warning
$iTunMOVI_data .= "	<key>copy-warning</key>
	<string>$copy_warning</string>
";
# set studio
$iTunMOVI_data .= "	<key>studio</key>
	<string>$studio</string>
";

# assigned a default producer
if ($producers[0] eq '') {
	@producers = ("$ripper");
}

# Process cast array
if ($cast[0] ne '') {
	$iTunMOVI_data .= "	<key>cast</key>
	<array>
";
}

foreach $actor (@cast) {
	$iTunMOVI_data .= "		<dict>
			<key>name</key>
			<string>$actor</string>
		</dict>
";
}

if ($cast[0] ne '') {
	$iTunMOVI_data .= "	</array>
";
}

# Process directors array
if ($directors[0] ne '') {
	$iTunMOVI_data .= "	<key>directors</key>
	<array>
";
}

foreach $director (@directors) {
	$iTunMOVI_data .= "		<dict>
			<key>name</key>
			<string>$director</string>
		</dict>
";
}

if ($directors[0] ne '') {
	$iTunMOVI_data .= "	</array>
";
}

# Process producers array
if ($producers[0] ne '') {
	$iTunMOVI_data .= "	<key>producers</key>
	<array>
";
}

foreach $producer (@producers) {
	$iTunMOVI_data .= "		<dict>
			<key>name</key>
			<string>$producer</string>
		</dict>
";
}

if ($producers[0] ne '') {
	$iTunMOVI_data .= "	</array>
";
}

# Process codirectors array
if ($codirectors[0] ne '') {
	$iTunMOVI_data .= "	<key>codirectors</key>
	<array>
";
}

foreach $codirector (@codirectors) {
	$iTunMOVI_data .= "		<dict>
			<key>name</key>
			<string>$codirector</string>
		</dict>
";
}

if ($codirectors[0] ne '') {
	$iTunMOVI_data .= "	</array>
";
}

# Process screenwriters array
if ($screenwriters[0] ne '') {
	$iTunMOVI_data .= "	<key>screenwriters</key>
	<array>
";
}

foreach $screenwriter (@screenwriters) {
	$iTunMOVI_data .= "		<dict>
			<key>name</key>
			<string>$screenwriter</string>
		</dict>
";
}

if ($screenwriters[0] ne '') {
	$iTunMOVI_data .= "	</array>
";
}

# set footer
$iTunMOVI_data .= "</dict>
</plist>
";

# print output
if ( $print ) {
	print "$iTunMOVI_data\n";
}

# check if file they want to write to can be opened
if ( $file ) {
	open(FILE, "$file") or die "Cannot open file <$file>: $!";
	close(FILE);
}

# do the AtomicParsley write
if ( $file && $write ) {
	`AtomicParsley \"$file\" --rDNSatom \'$iTunMOVI_data\' name=iTunMOVI domain=com.apple.iTunes --overWrite`;
}
