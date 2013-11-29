#!/usr/bin/env perl

use strict;
use warnings;

$\ = "\n";

my $conf = 'random-map.properties';
my $out  = 'run-once.properties';
my @maps = qw{default empty cheeser map01 map02 map03};

my $map = $maps[rand @maps];

`cp $conf $out`;
`echo 'map=$map' >>$out`;
