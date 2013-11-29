#!/usr/bin/env perl

use warnings;
use strict;

$\ = "\n";

chdir('local-runner');

my $wins = 0;
my $crashes = 0;

for (my $total = 1;; ++$total) {
    `./random-map.pl`;
    `./run-once.pl`;

    my ($seed, $place, $points, $verdict);
    open my $result, '<', 'result.txt';
    for (<$result>) {
        if (/SEED (\d+)/) {
            $seed = $1;
        }
        elsif (/(\S+) (\S+) (\S+)/) {
            $place   = $1;
            $points  = $2;
            $verdict = $3;
            last;
        }
    }

    if ($place == 1) {
        $wins += 1;
    }

    if ($verdict ne 'OK') {
        $crashes += 1;
        print "seed = $seed crash";
    }

    print "$total: ${\(100 * $wins / $total)}% wins, $crashes crashes";
}

`rm result.txt run-once.properties`;
