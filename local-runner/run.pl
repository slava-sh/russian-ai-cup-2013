#!/usr/bin/env perl

use warnings;
use strict;

$\ = "\n";

chdir('local-runner');

my $conf = 'random-map.properties';
my $out  = 'run-once.properties';
my @maps = qw{default empty cheeser map01 map02 map03};

my $wins = 0;
my $pluses = 0;
my $crashes = 0;

for (my $total = 1;; ++$total) {
    my $map = $maps[$total % scalar @maps];
    `cp $conf $out`;
    `echo 'map=$map'    >>$out`;
    `echo 'seed=$total' >>$out`;

    `./run-once.pl 2>../game.log`;

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

    if ($place <= 2) {
        $pluses += 1;
    }

    if ($verdict ne 'OK') {
        $crashes += 1;
        print "seed = $seed crash";
    }

    printf "%d: %d %0.3f%% wins, %0.3f%% pluses\n", $total, $place, 100 * $wins / $total, 100 * $pluses / $total;
}

`rm result.txt run-once.properties`;
