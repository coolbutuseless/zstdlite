  t = (int)(last - first);
  for(ssize = 0, limit = ss_ilg((int)(last - first));;) {
    if(limit-- == 0) { ss_heapsort(Td, PA, first, (int)(last - first)); }
          last = a, depth += 1, limit = ss_ilg((int)(a - first));
          STACK_PUSH(first, a, depth + 1, ss_ilg((int)(a - first)));
          last = a, depth += 1, limit = ss_ilg((int)(a - first));
      if((s = (int)(a - first)) > (t = (int)(b - a))) { s = t; }
      if((s = (int)(a - first)) > (t = (int)(b - a))) { s = t; }
      if((s = (int)(d - c)) > (t = (int)(last - d - 1))) { s = t; }
      if((s = (int)(d - c)) > (t = (int)(last - d - 1))) { s = t; }
          STACK_PUSH(b, c, depth + 1, ss_ilg((int)(c - b)));
          STACK_PUSH(b, c, depth + 1, ss_ilg((int)(c - b)));
          first = b, last = c, depth += 1, limit = ss_ilg((int)(c - b));
          STACK_PUSH(b, c, depth + 1, ss_ilg((int)(c - b)));
          STACK_PUSH(b, c, depth + 1, ss_ilg((int)(c - b)));
          first = b, last = c, depth += 1, limit = ss_ilg((int)(c - b));
        limit = ss_ilg((int)(last - first));
  l = (int)(middle - first), r = (int)(last - middle);
  l = (int)(middle - first), r = (int)(last - middle);
    for(a = first, len = (int)(middle - first), half = len >> 1, r = -1;
  ss_blockswap(buf, first, (int)(middle - first));
  ss_blockswap(buf, middle, (int)(last - middle));
    for(m = 0, len = MIN((int)(middle - first), (int)(last - middle)), half = len >> 1;
    for(m = 0, len = MIN((int)(middle - first), (int)(last - middle)), half = len >> 1;
      (bufsize < (limit = ss_isqrt((int)(last - first))))) {
    curbufsize = (int)(last - (a + SS_BLOCKSIZE));
  t = (int)(last - first);
    if((s = (int)(a - first)) > (t = (int)(b - a))) { s = t; }
    if((s = (int)(a - first)) > (t = (int)(b - a))) { s = t; }
    if((s = (int)(d - c)) > (t = (int)(last - d - 1))) { s = t; }
    if((s = (int)(d - c)) > (t = (int)(last - d - 1))) { s = t; }
  v = (int)(b - SA - 1);
      ISA[s] = (int)(d - SA);
      ISA[s] = (int)(d - SA);
  v = (int)(b - SA - 1);
      if(lastrank != rank) { lastrank = rank; newrank = (int)(d - SA); }
    if(lastrank != rank) { lastrank = rank; newrank = (int)(e - SA); }
      if(lastrank != rank) { lastrank = rank; newrank = (int)(d - SA); }
  int incr = (int)(ISAd - ISA);
  for(ssize = 0, limit = tr_ilg((int)(last - first));;) {
        tr_partition((int)(ISAd - incr), first, first, last, &a, &b, (int)(last - SA - 1));
          for(c = first, v = (int)(a - SA - 1); c < a; ++c) { ISA[*c] = v; }
          for(c = a, v = (int)(b - SA - 1); c < b; ++c) { ISA[*c] = v; }
            STACK_PUSH5(ISAd, b, last, tr_ilg((int)(last - b)), trlink);
            last = a, limit = tr_ilg((int)(a - first));
            first = b, limit = tr_ilg((int)(last - b));
            STACK_PUSH5(ISAd, first, a, tr_ilg((int)(a - first)), trlink);
            first = b, limit = tr_ilg((int)(last - b));
            last = a, limit = tr_ilg((int)(a - first));
          tr_copy(ISA, SA, first, a, b, last, (int)(ISAd - ISA));
          tr_partialcopy(ISA, SA, first, a, b, last, (int)(ISAd - ISA));
          do { ISA[*a] = (int)(a - SA); } while((++a < last) && (0 <= *a));
          next = (ISA[*a] != ISAd[*a]) ? tr_ilg((int)(a - first + 1)) : -1;
          if(++a < last) { for(b = first, v = (int)(a - SA - 1); b < a; ++b) { ISA[*b] = v; } }
          if(trbudget_check(budget, (int)(a - first))) {
      tr_heapsort(ISAd, first, (int)(last - first));
      next = (ISA[*a] != v) ? tr_ilg((int)(b - a)) : -1;
      for(c = first, v = (int)(a - SA - 1); c < a; ++c) { ISA[*c] = v; }
      if(b < last) { for(c = a, v = (int)(b - SA - 1); c < b; ++c) { ISA[*c] = v; } }
      if((1 < (b - a)) && (trbudget_check(budget, (int)(b - a)))) {
      if(trbudget_check(budget, (int)(last - first))) {
        limit = tr_ilg((int)(last - first)), ISAd += incr;
          else { skip = (int)(first - last); }
            if(0 <= c2) { BUCKET_B(c2, c1) = (int)(k - SA); }
        BUCKET_A(c2) = (int)(k - SA);
            if(0 <= c2) { BUCKET_B(c2, c1) = (int)(k - SA); }
        BUCKET_A(c2) = (int)(k - SA);
  return (int)(orig - SA);
          if ((s & mod) == 0) indexes[s / (mod + 1) - 1] = (int)(j - SA);
            if(0 <= c2) { BUCKET_B(c2, c1) = (int)(k - SA); }
    if (((n - 1) & mod) == 0) indexes[(n - 1) / (mod + 1) - 1] = (int)(k - SA);
      if ((s & mod) == 0) indexes[s / (mod + 1) - 1] = (int)(i - SA);
        BUCKET_A(c2) = (int)(k - SA);
          if ((s & mod) == 0) indexes[s / (mod + 1) - 1] = (int)(k - SA);
  return (int)(orig - SA);
