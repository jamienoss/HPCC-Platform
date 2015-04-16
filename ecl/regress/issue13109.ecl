r1 := { unsigned id };

MyRec := RECORD
    UNSIGNED2 uv;
    STRING10   sv;
END;

MyRec2 := RECORD(myRec)
    DATASET(r1) child;
END;

SomeFile := DATASET([{0x001,'GAVIN'},
                     {0x301,'JAMES'},
                     {0x301,'ABSALOM'},
                     {0x301,'BARNEY'},
                     {0x301,'CHARLIE'},
                     {0x301,'JETHROW'},
                     {0x001,'CHARLIE'},
                     {0x301,'DANIEL'},
                     {0x201,'Jim'}
                    ],MyRec);

p := PROJECT(SomeFile, TRANSFORM(myRec2, SELF := LEFT; SELF := []));
buildindex(NOFOLD(p), { p }, 'REGRESS:dummyIndex3',overwrite);
