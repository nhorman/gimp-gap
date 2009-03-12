#!/bin/sh
# audioconvert_to_wav.sh
#
# 2003.09.14 hof created by hof (Wolfgang Hofer)
#
SCRIPTNAME=`basename "$0"`
LAME=lame
SOX=sox

p_usage()
{
   echo ""
   echo "usage: $SCRIPTNAME --in audiofile --out wavefile [--resample samplerate_in_HZ]"
   echo ""
   echo "this script calls the lame for MP3 decoding, and sox for resampling or other conversions"
   echo "the resulting output file will be a RIFF wavfile "
   echo " " 
   echo "example1: (keep original samplerate)"
   echo "  $SCRIPTNAME --in kodachrome.mp3 --out kodachrome.wav"
   echo ""
   echo " " 
   echo "example2: (set samplerate)"
   echo "  $SCRIPTNAME --in kodachrome.mp3 --out kodachrome.wav --resample 44100"
   echo ""
   echo ""
}


# ---------------- 
# MAIN
# -----------------

# debug feedback input parameters
echo "$@"


EXPECT_RESAMPLE="NO"
EXPECT_INFILE="NO"
EXPECT_OUTFILE="NO"
IN_FILE=""
OUT_FILE=""
RESAMPLE_RATE=""
for ARG in "$@"
do
  if [ $EXPECT_RESAMPLE = "YES" ]
  then
    RESAMPLE_RATE=$ARG
    EXPECT_RESAMPLE="NO"
  else
    if [ $EXPECT_INFILE = "YES" ]
    then
      EXPECT_INFILE="NO"
      if [ -f "$ARG" ]
      then
        IN_FILE=$ARG
      else
	echo "file not found $ARG  .. will be ignored"
	exit 99
      fi
    else
      if [ $EXPECT_OUTFILE = "YES" ]
      then
        OUT_FILE=$ARG
        EXPECT_OUTFILE="NO"
      else
        case $ARG in
        -resample)   EXPECT_RESAMPLE="YES";;
        --resample)  EXPECT_RESAMPLE="YES";;
        -rate)       EXPECT_RESAMPLE="YES";;
        --rate)      EXPECT_RESAMPLE="YES";;
        -in)         EXPECT_INFILE="YES";;
        --in)        EXPECT_INFILE="YES";;
        -out)        EXPECT_OUTFILE="YES";;
        --out)       EXPECT_OUTFILE="YES";;
        -*)       p_usage; exit 99;;
        *)        p_usage; exit 99;;
        esac
      fi
    fi
  fi
done

if [ "x$IN_FILE" = "x" ]
then
  p_usage
  echo
  echo "No inputfile was supplied. (please use parmeter --in)"
  exit 99
fi;

if [ "x$OUT_FILE" = "x" ]
then
  OUT_FILE="${IN_FILE}.out.wav"
  echo
  echo "No outputfile was supplied. (using $OUT_FILE per default)"
fi;

type $SOX
if [ $? != 0 ]
then
  echo "$SOX is not installed (exiting now)"
  exit 1
fi


# check where to find lame
type $LAME
if [ $? != 0 ]
then
  echo "$LAME is not installed (exiting now)"
  exit 1
fi

# ---------------------
# START of CONVERT PART
# ---------------------
rm -f "$OUT_FILE"
if [ -f "$OUT_FILE" ]
then
  echo "could not write file: $OUT_FILE (check permissions)"
  exit 1
fi

# first try to convert $IN_FILE using sox
# (sox can handle a lot of audio fileformats,
#  but not MP3)

if [ "x$RESAMPLE_RATE" = "x" ]
then 
  $SOX  "$IN_FILE"  -t wav  "$OUT_FILE"
else
  $SOX  "$IN_FILE"  -t wav  "$OUT_FILE" rate -h $RESAMPLE_RATE
fi

if [ -s "$OUT_FILE" ]
then
  exit 0
fi


if [ "x$RESAMPLE_RATE" = "x" ]
then 
  # assume $IN_FILE is an MP3 and try to decode to WAV using lame
  $LAME  --decode "$IN_FILE" "$OUT_FILE"
else
   TMP_WAV="${IN_FILE}.tmp_mp3_to.wav"
   $LAME  --decode "$IN_FILE" "$TMP_WAV"
   $SOX  "$TMP_WAV"  -t wav "$OUT_FILE" rate -h $RESAMPLE_RATE
   rm -f "$TMP_WAV"
fi


if [ -s "$OUT_FILE" ]
then
  exit 0
fi

echo "could not convert $IN_FILE to 16 bit wav format"
exit 2
