#!/usr/bin/perl

####################################################################################
#   coolreader-ng, converter of Android hyphenation patterns to crengine-ng format #
#    url: https://android.googlesource.com/platform/external/hyphenation-patterns/ #
#   Copyright © 2022 by Aleksey Chernov <valexlin@gmail.com>                       #
#                                                                                  #
#   This program is free software; you can redistribute it and/or                  #
#   modify it under the terms of the GNU General Public License                    #
#   as published by the Free Software Foundation; either version 2                 #
#   of the License, or (at your option) any later version.                         #
#                                                                                  #
#   This program is distributed in the hope that it will be useful,                #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of                 #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                  #
#   GNU General Public License for more details.                                   #
#                                                                                  #
#   You should have received a copy of the GNU General Public License              #
#   along with this program; if not, write to the Free Software                    #
#   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA. #
####################################################################################

use warnings;
use strict;


sub makeConversion($$$$);
sub writePattern($$);
sub my_chomp;

# Locale names
my %LocaleNames = (
  'af' => 'Afrikaans',
  'am' => 'Amharic',
  'as' => 'Assamese',
  'be' => 'Belarusian',
  'bg' => 'Bulgarian',
  'bn' => 'Bengali',
  'cs' => 'Czech',
  'cu' => 'Church Slavic',
  'cy' => 'Welsh',
  'da' => 'Danish',
  'de' => 'German',
  'el' => 'Greek, Modern',
  'en' => 'English',
  'en-gb' => 'British English',
  'en-us' => 'US English',
  'eo' => 'Esperanto',
  'es' => 'Spanish',
  'eu' => 'Basque',
  'et' => 'Estonian',
  'fr' => 'French',
  'ga' => 'Irish',
  'gl' => 'Galician',
  'gu' => 'Gujarati',
  'hi' => 'Hindi',
  'hr' => 'Croation',
  'hu' => 'Hungarian',
  'hy' => 'Armenian',
  'it' => 'Italian',
  'ka' => 'Georgian',
  'kn' => 'Kannada',
  'la' => 'Latin',
  'lt' => 'Lithuanian',
  'lv' => 'Latvian',
  'ml' => 'Malayalam',
  'mn' => 'Mongolian',
  'mn-cyrl' => 'Mongolian Cyrillic',
  'mr' => 'Marathi',
  'nb' => 'Bokmål, Norwegian',
  'nl' => 'Dutch',
  'nn' => 'Norwegian Nynorsk',
  'or' => 'Oriya',
  'pa' => 'Punjabi',
  'pt' => 'Portuguese',
  'ru' => 'Russian',
  'sa' => 'Sanskrit',
  'sk' => 'Slovak',
  'sl' => 'Slovenian',
  'sq' => 'Albanian',
  'sv' => 'Swedish',
  'ta' => 'Tamil',
  'te' => 'Telugu',
  'tk' => 'Turkmen',
  'uk' => 'Ukrainian',
  'und-ethi' => 'Ethiopic script'
);

# hyphenmins for various locales
my %HyphenMins = (
  'cs' => '2,3',
  'cy' => '2,3',
  'el' => '1,1',
  'en' => '2,3',
  'en-gb' => '2,3',
  'en-us' => '2,3',
  'et' => '2,3',
  'fr' => '2,1',
  'ga' => '2,3',
  'hy' => '1,2',
  'ka' => '1,2',
  'pt' => '2,3',
  'sk' => '2,3'
);

if (scalar(@ARGV) < 2) {
    print("Hyphenation pattern converter\n");
    print("usage: $0 <lang_subdir> <dstfile.pattern> [<variant>]\n");
    exit -1;
}

my $lang_subdir = $ARGV[0];
my $out_filename = $ARGV[1];
my $lang_variant;

if (scalar(@ARGV) > 2) {
    $lang_variant = $ARGV[2];
}

my $indirh;
my $outfh;

if (!opendir($indirh, $lang_subdir)) {
    print "Cannot open director \"${lang_subdir}\"\n";
    exit 1;
}

if (!open($outfh, ">", $out_filename)) {
    closedir($indirh);
    print "Cannot create file \"${out_filename}\"!\n";
    exit 1;
}

print "$0: converting ${lang_subdir}/* to ${out_filename}...\n";
my $count = makeConversion($lang_subdir, $lang_variant, $indirh, $outfh);
if ($count > 0) {
    print "$count patterns saved.\n";
} else {
    print "0 patterns found or broken source file!\n";
}

close($outfh);
closedir($indirh);

# End of main.
1;

sub makeConversion($$$$) {
    my ($src_dir, $variant, $indirh, $outfh) = @_;
    my $res = 0;
    my ($lang_tag, $lang_name);
    my ($fname, @patfnames, $patfname_count, $patfname, $licfname, $licname);
    my ($hyphenmins, $lefthyphenmin, $righthyphenmin);
    # find license & pattern file
    @patfnames = ();
    $patfname_count = 0;
    while (defined($fname = readdir($indirh))) {
        if ($fname =~ /^hyph-.*\.pat\.txt$/) {
            push(@patfnames, $fname);
            $patfname_count++;
        } elsif ($fname =~ /^NOTICE$/) {
            $licfname = $fname
        } elsif ($fname =~ /^LICENSE$/) {
            $licfname = $fname
        } elsif ($fname =~ /^MODULE_LICENSE_(.*)$/) {
            $licname = $1;
        }
    }
    if (0 == $patfname_count) {
        printf "Pattern file not fount!\n";
        return 0;
    }
    if (length($variant) > 0) {
        foreach (@patfnames) {
            if (m/^hyph-${variant}\.pat\.txt$/) {
                $patfname = $_;
            }
        }
    } else {
        $patfname = pop(@patfnames);
    }
    if (!defined($patfname)) {
        printf "Pattern file not fount!\n";
        return 0;
    }
    if (!defined($licname)) {
        printf "License not fount!\n";
        return 0;
    }
    if (!defined($licfname)) {
        printf "License file not fount!\n";
        return 0;
    }
    if ($patfname =~ m/^hyph-(.*)\.pat\.txt$/) {
        $lang_tag = $1;
        if ($lang_tag =~ /^(.*)-[0-9]{2,4}$/) {
            $lang_tag = $1;
        }
        $lang_name = $LocaleNames{$lang_tag};
    }
    if (!defined($lang_tag)) {
        printf "Can't detect language tag!\n";
        return 0;
    }
    if (!defined($lang_name)) {
        printf "Can't detect language name!\n";
        return 0;
    }

    my ($patfh, $licfh);
    if (!open($patfh, "<", "${src_dir}/${patfname}")) {
        print "Cannot open pattern file \"${patfname}\"!\n";
        return 0;
    }
    if (!open($licfh, "<", "${src_dir}/${licfname}")) {
        print "Cannot open license file \"${licfname}\"!\n";
        return 0;
    }

    $hyphenmins = $HyphenMins{$lang_tag};
    $hyphenmins = "2,2" if (!defined($hyphenmins));
    if ($hyphenmins =~ m/^\s*(\d)\s*,\s*(\d)\s*$/) {
        $lefthyphenmin = $1;
        $righthyphenmin = $2;
    }
    $lefthyphenmin = 2 if (!defined($lefthyphenmin));
    $righthyphenmin = 2 if (!defined($righthyphenmin));

    print "pattern file: ${patfname}\n";
    print "license: ${licname}\n";
    print "license file: ${licfname}\n";
    print "language: ${lang_tag} => ${lang_name}\n";

    print $outfh ("<?xml version=\"1.0\" encoding=\"utf8\"?>\n\n" .
                  "<!-- Hyphenations description for crengine-ng -->\n" .
                  "<!-- Autogenerated based on android 'hyphenation-patterns' repository -->\n" .
                  "<!-- URL: https://android.googlesource.com/platform/external/hyphenation-patterns/ -->\n" .
                  "<!-- Original pattern file: ${patfname} -->\n" .
                  "<!-- License: ${licname} -->\n\n");

    print $outfh ("<!--\n");
    foreach (<$licfh>) {
        my_chomp;
        print $outfh ($_ . "\n");
    }
    print $outfh ("-->\n\n");
    close($licfh);

    print $outfh ("<HyphenationDescription lang=\"${lang_tag}\" title=\"${lang_name}\"");
    print $outfh (" lefthyphenmin=\"${lefthyphenmin}\" righthyphenmin=\"${righthyphenmin}\">\n");

    foreach (<$patfh>) {
        my_chomp;
        if (length($_) > 0) {
            writePattern($outfh, $_);
            $res++;
        }
    }
    close($patfh);

    print $outfh ("</HyphenationDescription>\n");
    return $res;
}

sub writePattern($$) {
    my ($fh, $pattern) = @_;
    if ($pattern =~ m/^\.(.*)$/) {
        $pattern = " $1";
    }
    if ($pattern =~ m/^(.*)\.$/) {
        $pattern = "$1 ";
    }
    print $fh ("  <pattern>${pattern}</pattern>\n");
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
