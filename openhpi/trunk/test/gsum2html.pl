#!/usr/bin/perl

#       $Id$
 
#  (C) Copyright IBM Corp. 2004
 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
#  file and program are licensed under a BSD style license.  See
#  the Copying file included with the OpenHPI distribution for
#  full licensing terms.
 
#  Authors:
#      Sean Dague <http://dague.net/sean>

use strict;
use HTML::Entities;

foreach my $file (@ARGV) {
#    my $outfile = $file . ".html";
    open(IN,"$file");
    my @lines = <IN>;
    close(IN);
    
    my $html = make_html_head($file);
    $html .= make_html_body(@lines);
    $html .= make_html_tail();

    print $html;
#    open(OUT,">$outfile");
#    print OUT $html;
#    close(OUT);
}

sub make_html_head {
    my $title = shift;
    $title =~ s/.*\/(.*)\.summary$/$1/;
    return <<END;
<html>
<head><title>GCOV Report for $title</title>
<!--#include virtual="/openhpi.css" -->
</head>
<body>
<table>
<tr>
<!--#include virtual="/sidebar.html" -->
<td valign="top">
<h1>GCOV Summary for $title</h1>
END
}

sub make_html_tail {
    return <<END;
</table>
</td></tr></table>
</body>
</html>
END
}

sub set_status {
    my $exec = shift;
    if($exec eq "-") {
        return "na";
    } elsif($exec eq "#####") {
        return "notexec";
    } elsif($exec < 10) {
        return "low";
    } else {
        return "good";
    }
}

sub make_html_body {
    my @lines = @_;
    my $html;
    foreach my $line (@lines) {
        if($line =~ /^Function/) {
            $line =~ s/.*`([^']+).*/$1/;
            # close the last table
            $html .= "</table>\n";
            $html .= "<h2 class='function'>$line</h2>\n<table class='report'>\n";
        } elsif($line =~ /^File/) {
            $line =~ s/.*`([^']+).*/$1/;
            $html .= "<h1 class='file'>$line</h1>\n<table class='report'>\n"
        } elsif ($line =~ /^No/) {
            $html .= "<tr class='na'><td>$line</td></tr>\n";
        } elsif ($line =~ /(\d+\.\d{2})%/) {
            my $per = $1;
            my $status = "bad";
            if($per >= 100) {
                $status = "great";
            } elsif($per > 80) {
                $status = "good";
            } elsif($per > 50) {
                $status = "ok";
            }
            $html .= "<tr class='$status'><td>$line</td></tr>\n";
        }
    }
    return $html;
}
