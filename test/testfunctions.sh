#
# Common functions for CANT test harness
#
# $Id: testfunctions.sh,v 1.3 2002-03-29 11:14:01 gnb Exp $
#

CANT="../../src/cant"
VERBOSE=no
DEBUG=no

usage ()
{
    echo "Usage: runtest [--verbose] [--cant=cantexe]"
    exit 1
}

while [ $# -gt 0 ]; do
    case "$1" in
    --verbose) VERBOSE=yes ;;
    --debug) DEBUG=yes ;;
    --cant=*) CANT=`echo "$1"|sed -e 's|^[^=]*=||'` ;;
    --cant) CANT="$2" ; shift ;;
    --help) usage ;;
    -*) usage ;;
    *) usage ;;
    esac
    shift
done

if [ $DEBUG = yes ]; then
    VERBOSE=yes
    set -x
fi

test -f globals.xml && CANT="$CANT --globals-file globals.xml"

CANTOUT=cant$$.out
trap "/bin/rm -f $CANTOUT" 0

CANTTMP1=cant$$a.tmp
CANTTMP2=cant$$a.tmp
trap "/bin/rm -f $CANTTMP1 $CANTTMP2" 0


failed ()
{
    echo "runtest: FAILED"
    exit 1
}

vmessage ()
{
    test $VERBOSE = yes && echo "runtest: $*"
}

vdiff ()
{
    local FILE1="$1"
    local FILE2="$2"
    
    if [ $VERBOSE = yes ]; then
	diff -u $FILE1 $FILE2 || failed
    else
	diff $FILE1 $FILE2 >/dev/null || failed
    fi
}

cant ()
{
    local status
    
    vmessage "Running: $CANT $@"
    /bin/rm -f $CANTOUT
    $CANT $@ >$CANTOUT 2>&1
    status=$?
    test $VERBOSE = yes && cat $CANTOUT
    test $status = 0 || failed
}

cant_status ()
{
    vmessage "Running: $CANT $@"
    /bin/rm -f $CANTOUT
    $CANT $@ >$CANTOUT 2>&1
    CANTSTATUS=$?
    test $VERBOSE = yes && cat $CANTOUT
}

check_status ()
{
    vmessage "Checking cant exit status is $1"
    case "$1" in
    success|yes)
	test "$CANTSTATUS" = 0 || failed
	;;
    failure|no)
	test "$CANTSTATUS" != 0 || failed
	;;
    [0-9]*)
	test "$CANTSTATUS" = "$1" || failed
	;;
    esac
}

check_output_re ()
{
    vmessage "Checking output against expression $*"
    grep "$*" $CANTOUT >/dev/null || failed
}

check_output_file ()
{
    vmessage "Checking output against file $1"
    vdiff "$1" $CANTOUT
}

start_test ()
{
    echo "==========================="
    echo "runtest: $* test${VALUE:+, value=}$VALUE${EXPECTED:+, expected=}$EXPECTED"
}


hexcanon ()
{
    tr '[:upper:]' '[:lower:]' | sed -e 's|^[ 	]\+||' -e 's|[ 	]\+|\
|g'
}

check_file_hex ()
{
    local FILE="$1"
    local CONTENTS="$2"
    
    /bin/rm -f $CANTTMP1 $CANTTMP2
    echo "$CONTENTS" | hexcanon > $CANTTMP1
    cat "$FILE" | hexcanon > $CANTTMP2
    vmessage "Checking contents of file $FILE"
    vdiff $CANTTMP1 $CANTTMP2
}

check_file_exists ()
{
    vmessage "Checking that file $1 exists"
    test -f "$1" || failed
}

check_file_notexists ()
{
    vmessage "Checking that file $1 doesn't exist"
    test -f "$1" && failed || return 0
}

check_file_executable ()
{
    vmessage "Checking that file $1 is executable"
    test -x "$1" || failed
}

check_file_contents ()
{
    vmessage "Checking file $1 against file $2"
    vdiff "$2" "$1"
}

run_prog ()
{
    vmessage "Running program $*"
    if [ $VERBOSE = yes ]; then
	$* || failed
    else
	$* >/dev/null || failed
    fi
}
