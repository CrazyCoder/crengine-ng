#!/usr/bin/perl

###########################################################################
#   coolreader-ng, converter of TeX hyphenation files to FBReader format  #
#   Copyright © 2022 by coolreader-ng authors                             #
#   Copyright © 2011 by Vadim Lopatin                                     #
#                                                                         #
#   This program is free software; you can redistribute it and/or         #
#   modify it under the terms of the GNU General Public License           #
#   as published by the Free Software Foundation; either version 2        #
#   of the License, or (at your option) any later version.                #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the Free Software           #
#   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            #
#   MA 02110-1301, USA.                                                   #
###########################################################################

use warnings;

sub makeConversion($$$);
sub outPattern($$);
sub my_chomp;

if (scalar(@ARGV) < 2) {
    print("Hyphenation pattern converter\n");
    print("usage: $0 <srclistfile.tex> <dstfile.pattern>\n");
    exit -1;
}

my $in_filename = $ARGV[0];
my $out_filename = $ARGV[1];

my $infh;
my $outfh;
if (!open($infh, "<", $in_filename)) {
    print "Cannot open file ${in_filename}\n";
    exit 1;
}
if (!open($outfh, ">", $out_filename)) {
    close($infh);
    print "Cannot create file ${out_filename}\n";
    exit 1;
}

print "$0: converting ${in_filename} to ${out_filename}...\n";
my $count = makeConversion($in_filename, $infh, $outfh);
if ($count > 0) {
    print "$count patterns saved.\n";
} else {
    print "0 patterns found or broken source file!\n";
}

close($outfh);
close($infh);

# End of main.
1;


sub makeConversion($$$) {
    my ($srcFileName, $infh, $outfh) = @_;
    print $outfh ("<?xml version=\"1.0\" encoding=\"utf8\"?>\n" .
                  "<!--\n" .
                  "       hyphenations description for FBReader/CoolReader\n" .
                  "       from the original file: ${srcFileName}\n" .
                  "\n");
    my $state = 0;
    my $count = 0;
    my $word;
    my $ch;
    foreach (<$infh>) {
        my_chomp;
        if ($state == 0) {
            if (m/^%.*$/) {
                print $outfh ("$_\n");
                next;
            }
            if (m/^\\patterns\{.*$/) {
                print $outfh ("-->\n");
                print $outfh ("<HyphenationDescription>\n");
                $state = 1;
                next;
            }
        } else {
            if (m/^%(.*)$/) {
                if (length($1) > 0) {
                    print $outfh ("<!-- $1 -->\n");
                }
                next;
            }
            $word = "";
            foreach $ch (split //) {
                if ($ch eq '}') {
                    print $outfh ("</HyphenationDescription>\n");
                    $state = 0;
                    last;
                }
                if ($ch =~ m/ |\t|%/) {
                    if (length($word) > 0) {
                        outPattern($outfh, $word);
                        $count++;
                        $word = "";
                    }
                    if ($ch =~ m/%/) {
                        last;
                    }
                } else {
                    $word = $word . $ch;
                }
            }
            if (length($word) > 0) {
                outPattern($outfh, $word);
                $count++;
            }
        }
    }
    return $count;
}

sub outPattern($$) {
    my ($outfh, $pattern) = @_;
    if ($pattern =~ m/^\.(.*)$/) {
        $pattern = " $1";
    }
    if ($pattern =~ m/^(.*)\.$/) {
        $pattern = "$1 ";
    }
    print $outfh ("  <pattern>${pattern}</pattern>\n");
}

# chomp version for '\r\n' or '\n' or '\r' at the same time
sub my_chomp
{
    my $res = 0;
    my $line;
    if (defined($_[0])) {
        $line = \$_[0];
    } else {
        $line = \$_;
    }
    my $len = length($$line);
    my $c;
    if ($len > 0) {
        $c = ord(substr($$line, $len - 1, 1)); 
        if ($c == 0x0A) {
            $$line = substr($$line, 0, $len - 1);
            $len--;
            $res++;
        }
    }
    if ($len > 0) {
        $c = ord(substr($$line, $len - 1, 1)); 
        if ($c == 0x0D) {
            $$line = substr($$line, 0, $len - 1);
            $res++;
        }
    }
    return $res;
}
