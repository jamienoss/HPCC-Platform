r := RECORD
    UNSIGNED i;
END;
ROW(TRANSFORM(r, SELF.i := 0));
