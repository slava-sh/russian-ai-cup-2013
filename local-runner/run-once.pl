#!/usr/bin/env perl

use warnings;
use strict;

$\ = "\n";

my $runner;
unless ($runner = fork) {
    `java -cp ".:*" -jar local-runner.jar run-once.properties`;
    print "local-runner.jar: $?";
}
else {
    my $strategy;
    unless ($strategy = fork) {
        for (;;) {
            `../MyStrategy 2>../game.log`;
            my $code = $? >> 8;
            if ($code != 17) {
                last;
            }
        }
        print "MyStrategy: $?";
    }
    else {
        waitpid $runner, 0;
        waitpid $strategy, 0;
    }
}
