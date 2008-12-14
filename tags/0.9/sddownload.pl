#!/usr/bin/perl -w

use strict;

use LWP::UserAgent;

my $ua = LWP::UserAgent->new(keep_alive => 1, env_proxy => 0);
$ua->agent('StarDust@Home scraper 0.1');

my $url = shift || die 'gimme url';
my $req = HTTP::Request->new(GET => $url);
my $res = $ua->request($req);
die if !$res->is_success;

die unless $res->content =~ /for\(var i = 1; i <= (\d+); i\+\+\) {/;
my $count = $1;

die unless $res->content =~ /full_filename\("(.*)001.jpg", i\)/;
my $baseurl = $1;

for (1..$count) {
	system("wget", $baseurl.sprintf("%03d", $_).".jpg");
}
