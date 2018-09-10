use strict;
use warnings;

use Data::Dumper;
use Path::Tiny;
use Test::More;

my $CLASS = 'JavaScript::V8::XS';

sub load_js_file {
    my ($file) = @_;

    my $path = Path::Tiny::path($file);
    my $js_code = $path->slurp_utf8();
    return $js_code;
}

sub save_tmp_file {
    my ($contents) = @_;

    my $path = Path::Tiny->tempfile();
    $path->spew_utf8($contents);
    return $path;
}

sub test_line_numbers {
    my $js_code = <<EOS;
var fail = true;

function d()
{
    if (fail) {
        throw new Error("failed");
    }
}

function c()
{
    if (fail) {
        return gonzo.length;
    }
}

function b()
{
    c();
}

function main() {
    b();
    return "ok";
}
EOS

    my $vm = $CLASS->new({save_messages => 1});
    ok($vm, "created $CLASS object that saves messages");

    my @js_files;
    my $js_file = save_tmp_file($js_code);
    push @js_files, $js_file;
    ok(1, "saved to tmp file '$js_file'");

    foreach my $js_file (@js_files) {
        my $js_code = load_js_file($js_file);
        $vm->eval($js_code, $js_file);
        ok(1, "loaded file '$js_file'");
    }

    my $name = 'main';
    my %types = (
        eval     => "%s()",
        dispatch_function_in_event_loop => "%s",
    );
    foreach my $type (sort keys %types) {
        my $format = $types{$type};
        $vm->reset_msgs();
        $vm->$type(sprintf($format, $name));
        my $msgs = $vm->get_msgs();
        # print STDERR Dumper($msgs);

        ok($msgs, "got messages from JS for type $type");
        next unless $msgs;

        ok($msgs->{stderr}, "got error messages from JS for type $type");
        next unless $msgs->{stderr};

        my $contexts = $vm->parse_js_stacktrace($msgs->{stderr}, 2);
        ok($contexts, "got parsed stacktrace for type $type");
        next unless $contexts;

        my $context_no = 0;
        foreach my $context (@$contexts) {
            ++$context_no;
            like($context->{message}, qr/:.*(not |un)defined/,
                 "context $context_no contains error message for type $type");
            is(scalar @{ $context->{frames} }, 2,
               "context $context_no contains correct number of frames for type $type");
            my $frame_no = 0;
            foreach my $frame (@{ $context->{frames} }) {
                ++$frame_no;
                ok(exists $frame->{file}, "frame $context_no.$frame_no has member file for type $type");
                ok(exists $frame->{line}, "frame $context_no.$frame_no has member line for type $type");
                ok(exists $frame->{line_offset}, "frame $context_no.$frame_no has member line_offset for type $type");
                ok(ref( $frame->{lines} ) eq 'ARRAY', "frame $context_no.$frame_no has member lines as arrayref for type $type");
            }
        }

        # print STDERR Dumper($contexts);
    }
}

sub test_multiple_errors {
    my $js_fmt = <<EOS;
function f_%d () {
    setTimeout(function() {
        throw new Error("error1");
    }, 0);

    setTimeout(function() {
        return gonzo.length;
    }, 0);

    setTimeout(function() {
        throw new Error("error3");
    }, 0);
}
EOS

    my $vm = $CLASS->new({save_messages => 1});
    ok($vm, "created $CLASS object that saves messages");

    my $top = 2;
    my @js_files;
    foreach my $seq (1..$top) {
        my $js_code = sprintf($js_fmt, $seq);
        my $js_file = save_tmp_file($js_code);
        push @js_files, $js_file;
    }
    is(scalar @js_files, $top, "saved $top functions to tmp files");

    my @interesting_files;
    foreach my $js_file (@js_files) {
        my $js_code = load_js_file($js_file);
        $vm->eval($js_code, $js_file);
        ok(1, "loaded file '$js_file'");
        next if scalar @interesting_files > 0;
        push @interesting_files, $js_file;
    }

    my $name = 'f';
    my %types = (
        eval     => "%s_%d()",
        dispatch_function_in_event_loop => "%s_%d",
    );
    foreach my $type (sort keys %types) {
        my $format = $types{$type};
        $vm->reset_msgs();
        foreach my $seq (1..$top) {
            my $js_func = sprintf($format, $name, $seq);
            $vm->dispatch_function_in_event_loop($js_func);
        }
        my $msgs = $vm->get_msgs();
        # print STDERR Dumper($msgs);
        ok($msgs, "got messages from JS for type $type");
        next unless $msgs;

        ok($msgs->{stderr}, "got error messages from JS for type $type");
        next unless $msgs->{stderr};

        my $contexts = $vm->parse_js_stacktrace($msgs->{stderr}, 2, \@interesting_files);
        ok($contexts, "got parsed stacktrace for type $type");
        next unless $contexts;
        # print STDERR Dumper($contexts);
    }
}

sub main {
    use_ok($CLASS);

    test_line_numbers();
    test_multiple_errors();
    done_testing;
    return 0;
}

exit main();
