#!/bin/bash

# Process doxygen.warnings.log to generate sorted list of top offenders

# Flag to skip running doxygen
skipdoxy=${1:-""}

DIR=`dirname $0`
ROOT=`hg root`
# Final resting place for the log file
log=$DIR/doxygen.warnings.log


# First, we have to modify doxygen.conf to generate all the warnings
# (We also suppress dot graphs, so shorten the run time.)

if [ "$skipdoxy" == "" ]; then

    conf=$DIR/doxygen.conf

    sed -i.bak -E '/^EXTRACT_ALL |^HAVE_DOT |^WARNINGS /s/YES/no/' $conf
    rm -f $conf.bak

    echo -n "Rebuilding doxygen docs with full errors..."
    (cd $ROOT && ./waf --doxygen >/dev/null 2>&1)
    status=$?

    hg revert $conf

    if [ "$status" = "0" ]; then
	echo "Done."
    else
	echo "FAILED."
	exit 1
    fi

    mv $DIR/doxygen.log $log

else
    echo "Skipping doxygen run, using existing log file $log"
fi


# Analyze the log

# List of module directories (e.g, "src/core/model")
undocmods=$(                \
    grep "^$ROOT" "$log"  | \
    cut -d ':' -f 1       | \
    sed "s|$ROOT||g"      | \
    cut -d '/' -f 2-4     | \
    sort                  | \
    uniq -c               | \
    sort -nr                \
    )
    

# Number of directories
modcount=$(                         \
    echo "$undocmods"             | \
    wc -l                         | \
    sed 's/^[ \t]*//;s/[ \t]*$//'   \
    )

# For a function with multiple undocumented parameters,
# Doxygen prints the additional parameters on separate lines,
# so they don't show up in the totals above.
# Rather than work too hard to get the exact number,
# we just list the total here. 
addlparam=$(                                  \
    grep -v "^$ROOT" $log                   | \
    grep -v "not generated, too many nodes" | \
    grep "^  parameter '" $log              | \
    wc -l                                   | \
    sed 's/^[ \t]*//;s/[ \t]*$//'             \
    )

# Total number of warnings
warncount=$(echo "$undocmods" | \
    awk '{total += $1}; END {print total}' )
warncount=$((warncount + addlparam))

# List of files appearing in the log
undocfiles=$( \
    grep "^$ROOT" "$log"  | \
    cut -d ':' -f 1       | \
    sed "s|$ROOT||g"      | \
    cut -d '/' -f 2-      | \
    sort                  | \
    uniq -c               | \
    sort -k 2               \
    )

# Total number of files
filecount=$(                        \
    echo "$undocfiles"            | \
    wc -l                         | \
    sed 's/^[ \t]*//;s/[ \t]*$//'   \
    )

# Now we're ready to summarize the log

echo
echo "Report of Doxygen warnings"
echo "----------------------------------------"
echo
echo "(All counts are lower bounds.)"
echo
echo "Warnings by module/directory:"
echo
echo "Count Directory"
echo "----- ----------------------------------"
echo "$undocmods"
echo " $addlparam additional undocumented parameters."
echo "----------------------------------------"
printf "%6d total warnings\n" $warncount
printf "%6d directories with warnings\n" $modcount
echo
echo
echo "Warnings by file"
echo
echo "Count File" 
echo "----- ----------------------------------"
echo "$undocfiles"
echo "----------------------------------------"
printf "%6d files with warnings\n" $filecount
echo
echo
echo "Doxygen Warnings Summary"
echo "----------------------------------------"
printf "%6d directories\n" $modcount
printf "%6d files\n" $filecount
printf "%6d warnings\n" $warncount
