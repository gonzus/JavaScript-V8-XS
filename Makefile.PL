use strict;
use warnings;
use 5.008009;

use ExtUtils::MakeMaker;

my @V8_CC_OPTS;
my @V8_CC_DEFS;
my @V8_CC_WRNS;
my @V8_CC_INCS;
my @V8_LD_OPTS;
my @V8_LD_LIBS;

push @V8_CC_OPTS, qw< -O2 -std=c++14 >;
push @V8_CC_WRNS, qw< all extra no-unused-parameter >;
push @V8_LD_LIBS, qw< v8 v8_libbase v8_libplatform icuuc icui18n >;

if ($^O eq 'linux') {
    push @V8_CC_DEFS, qw< PERL_JAVASCRIPT_V8_XS_LINUX >;

    # these apply only on linux, where the V8 build has to ignore standard
    # include files and instead looks at its own internal libc++ files
    # TODO: double-check this is still needed
    # TODO: figure out a better place to install includes / libraries / blobs
    push @V8_CC_OPTS, qw< -nostdinc++ >;
    push @V8_CC_INCS, map { "/usr/include/$_/include" } qw< v8/libc++ v8/libc++abi >;
}

if ($^O eq 'darwin') {
    push @V8_CC_DEFS, qw< PERL_JAVASCRIPT_V8_XS_DARWIN >;
}

my $CC = 'c++';
my $LD = 'c++';

my $CC_OPTS = join(' ',
    (@V8_CC_OPTS),
    (map { "-D$_" } @V8_CC_DEFS),
    (map { "-W$_" } @V8_CC_WRNS),
    (map { "-I$_" } @V8_CC_INCS),
);
my $LD_OPTS = join(' ',
    (@V8_LD_OPTS),
    map { "-l$_" } @V8_LD_LIBS,
);

WriteMakefile(
    NAME           => 'JavaScript::V8::XS',
    VERSION_FROM   => 'lib/JavaScript/V8/XS.pm',
    ABSTRACT_FROM  => 'lib/JavaScript/V8/XS.pm',
    LICENSE        => 'mit',
    MIN_PERL_VERSION => 5.018000,
    PREREQ_PM      => {
        'JSON::PP'        => 0,
        'Text::Trim'      => 0,
        'XSLoader'        => 0,
        'ExtUtils::XSpp'  => '0.11',
    },
    TEST_REQUIRES  => {
        'Data::Dumper'    => 0,
        'JSON::PP'        => 0,
        'Path::Tiny'      => 0,
        'Scalar::Util'    => 0,
        'Test::More'      => 0,
        'Test::Output'    => 0,
        'Test::Exception' => 0,
        'Text::Trim'      => 0,
        'Time::HiRes'     => 0,
    },
    AUTHOR         => [
        'Gonzalo Diethelm (gonzus@cpan.org)',
    ],

    # compiler settings
    CC                 => "$CC $CC_OPTS",
    DEFINE             => '',
    depend             => { 'v8-perl.c' => 'JavaScript-V8-XS-Context.xsp' },

    # linker settings
    LD                 => "$LD $LD_OPTS",
    OBJECT             => '$(O_FILES)',

    # precompiler settings
    XSOPT              => '-C++ -hiertype',
    TYPEMAPS           => ['perlobject.map'],

    # repo settings
    META_MERGE         => {
        'meta-spec'    => { version => 2 },
        resources      => {
            repository => {
                type   => 'git',
                url    => 'git@github.com:gonzus/JavaScript-V8-XS',
                web    => 'https://github.com/gonzus/JavaScript-V8-XS',
            },
        },
    },
);
