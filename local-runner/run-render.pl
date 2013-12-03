#!/usr/bin/env perl

use warnings;
use strict;

$\ = "\n";

chdir('local-runner');

my $conf = 'local-runner-sync.properties';
my $out  = 'run-once.properties';

`cp $conf $out`;

`./run-once.pl`;

my ($seed, $place, $points, $verdict);
open my $result, '<', 'result.txt';
for (<$result>) {
    if (/SEED (\d+)/) {
        $seed = $1;
    }
    elsif (/(\S+) (\S+) (\S+)/) {
        $verdict = $3;
        last;
    }
}

if ($verdict ne 'OK') {
    print "seed = $seed crash";
}

`rm result.txt run-once.properties`;
