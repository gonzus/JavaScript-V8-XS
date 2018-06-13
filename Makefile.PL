use 5.008009;
use ExtUtils::MakeMaker;

my $V8_ROOT = '/Users/gdiethelm/src/v8';
my $V8_INCLUDE = "$V8_ROOT/include";
my $V8_LIB = "$V8_ROOT/out.gn/x64.release";
my @V8_LIBS = qw/ v8 v8_libbase v8_libplatform icuuc icui18n /;

my $CC = 'c++';
my $CC_OPTS = '--std=c++0x -Wreserved-user-defined-literal';
my $LD = 'c++';
my $LD_OPTS = "-L${V8_LIB} " . join(' ', map { "-l$_" } @V8_LIBS);

WriteMakefile(
    NAME           => 'JavaScript::V8::XS',
    VERSION_FROM   => 'lib/JavaScript/V8/XS.pm',
    ABSTRACT_FROM  => 'lib/JavaScript/V8/XS.pm',
    LICENSE        => 'mit',
    MIN_PERL_VERSION => 5.018000,
    PREREQ_PM      => {
        'ExtUtils::XSpp' => '0.11',
        # 'XSLoader'     => 0,
    },
    TEST_REQUIRES  => {
        'Data::Dumper' => 0,
        'Test::More'   => 0,
    },
    AUTHOR         => [
        'Gonzalo Diethelm (gonzus@cpan.org)',
    ],

    CC                 => "$CC $CC_OPTS",
    DEFINE             => '',
    INC                => "-I $V8_INCLUDE",

    LD                 => "$LD $LD_OPTS",
    OBJECT             => '$(O_FILES)',

    XSOPT              => '-C++ -hiertype',
    TYPEMAPS           => ['perlobject.map'], # TODO
    depend             => { 'v8-perl.c' => 'JavaScript-V8-XS-Context.xsp' },
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
