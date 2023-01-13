#!/bin/bash
cd "$(dirname "$0")" || exit

# AP location
AP=../build/AtomicParsley
if [ ! -f "${AP}" ]; then
  AP=./AtomicParsley
fi

# input location
INPUT=issue-32.mp4

# output location
OUTPUT=issue-32-reverseDNS.mp4
if [ -f "${OUTPUT}" ]; then
  rm "${OUTPUT}"
fi

# try to add many rDNS tags from input to output
"${AP}" "${INPUT}" \
  --title "I Recall" \
  --artist Cosmograf \
  --album "Heroic Materials" \
  --genre "Progressive Rock" \
  --tracknum 01/10 \
  --disk 01/01 \
  --year 2022 \
  --stik Normal \
  --sortOrder albumartist Cosmograf \
  --sortOrder artist Cosmograf \
  --albumArtist Cosmograf \
  --rDNSatom edc087c7-186c-411d-8285-0c3bb55aae5c "name=MusicBrainz Album Artist Id" domain=com.apple.iTunes \
  --rDNSatom efeeb1ac-cdd8-433b-bced-aa285de34025 "name=MusicBrainz Album Id" domain=com.apple.iTunes \
  --rDNSatom edc087c7-186c-411d-8285-0c3bb55aae5c "name=MusicBrainz Artist Id" domain=com.apple.iTunes \
  --rDNSatom 605f8821-fd66-49c4-82dc-3563e8e6d289 "name=MusicBrainz Track Id" domain=com.apple.iTunes \
  --rDNSatom 8b87507e-2a54-4438-baf3-046b0f099e8a "name=Acoustid Id" domain=com.apple.iTunes \
  --rDNSatom f6a879d9-33d1-4247-8e19-426cd932b3b5 "name=MusicBrainz Release Group Id" domain=com.apple.iTunes \
  --rDNSatom c530683b-07a2-4677-9990-6c5dac20b3f0 "name=MusicBrainz Release Track Id" domain=com.apple.iTunes \
  --rDNSatom XW "name=MusicBrainz Album Release Country" domain=com.apple.iTunes \
  --rDNSatom official "name=MusicBrainz Album Status" domain=com.apple.iTunes \
  --rDNSatom album "name=MusicBrainz Album Type" domain=com.apple.iTunes \
  --rDNSatom 0796520103412 name=BARCODE domain=com.apple.iTunes \
  --rDNSatom "Digital Media" name=MEDIA domain=com.apple.iTunes \
  --rDNSatom Latn name=SCRIPT domain=com.apple.iTunes \
  --rDNSatom "-3.36 dB" name=replaygain_album_gain domain=com.apple.iTunes \
  --rDNSatom 0.98791504 name=replaygain_album_peak domain=com.apple.iTunes \
  --rDNSatom "-0.07 dB" name=replaygain_track_gain domain=com.apple.iTunes \
  --rDNSatom 0.57928467 name=replaygain_track_peak domain=com.apple.iTunes \
  --rDNSatom "89.0 dB" name=replaygain_reference_loudness domain=com.apple.iTunes \
  -o "${OUTPUT}"

# display results
if [ -f "${OUTPUT}" ]; then
  "${AP}" "${OUTPUT}" -T 1 -t +
  rm "${OUTPUT}"
  echo "OK"
  exit 0
else
  echo "KO"
  exit 1
fi
