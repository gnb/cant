define(DATE,esyscmd(date +"%d %b %Y"))dnl
define(YEAR,esyscmd(date +"%Y"))dnl
define(BEGINHEAD,
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<HTML><HEAD>
include(_copyright.html)
<title>TITLE</title>
)dnl
define(ENDHEAD,
</HEAD>
)dnl
define(BEGINBODY,
<BODY BGCOLOR="#ffffff">
<TABLE BORDER=0 CELLSPACING=4>
  <TR>
    <TD COLSPAN=2 ALIGN=CENTER>
      <IMG WIDTH=406 HEIGHT=118 SRC="cant_banner.gif" ALT="CANT">
    </TD>
  </TR>
  <TR>
    <TD VALIGN=TOP>
include(toc.html.in)
    </TD>
    <TD>
    <H1>TITLE</H1>
)dnl
define(ENDBODY,
    </TD>
  </TR>
</TABLE>
<!-- <HR NOSHADE SIZE=4> -->
<BR><BR>
<CENTER>
<P CLASS="small">
Last updated: DATE.<BR>
&copy; 2001 Greg Banks. All Rights Reserved.<BR>
Ant theme clipart from <A HREF="http://www.arttoday.com/">ArtToday.com</A>
</P>
</CENTER>
</BODY></HTML>
)dnl
define(EMAILME,
$1 <A HREF="mailto:gnb@alphalink.com.au?Subject=CANT">gnb@alphalink.com.au</A>
)dnl
dnl Usage: THUMBNAIL(some/dir/fred,gif,[alternate text])
define(THUMBNAIL,
<A HREF="$1.$2"><IMG SRC="$1_t.$2" ALT="$3" BORDER=0></A>
)dnl
dnl Usage: BEGINDOWNLOAD ( DOWNLOAD(description,filename) )* ENDDOWNLOAD
define(BEGINDOWNLOAD,
<TABLE>
)dnl
define(ENDDOWNLOAD,
</TABLE>
)dnl
define(DOWNLOAD,
`  <TR>
    <TD VALIGN=TOP><B>$1</B></TD>
    <TD>
      <A HREF="$2">$2</A><BR>
      esyscmd(ls -l $2 | awk {print`\$'5}) bytes<BR>
      MD5 esyscmd(md5sum $2 | awk {print`\$'1})
    </TD>
  </TR>'
)dnl
