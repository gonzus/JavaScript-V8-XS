use strict;
use warnings;

use Data::Dumper;
use Test::More;
use Test::Output;
use JavaScript::V8::XS;

sub test_load {
    my $v8 = JavaScript::V8::XS->new();
    ok($v8, "created JavaScript::V8::XS object");

    my $text = 'Hello';
    my $number = 11;
    stderr_like sub { $v8->eval("'$text' + ' ' +  $number") },
                qr/$text.*$number/,
                "got correct output in stderr";
}

sub main {
    test_load();
    done_testing;
    return 0;
}

exit main();
