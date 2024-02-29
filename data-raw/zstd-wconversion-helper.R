
library(processx)
library(stringr)

zstd <- ozstd <- readLines("src/zstd/zstd.c")

comp_output <- processx::run("R", args <- c("CMD", "INSTALL", "--preclean", "--no-multiarch", "--with-keep.source", "."))
stderr  <- strsplit(comp_output$stderr, "\n")[[1]]


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Find and fix "-Wshorten-64-to-32" errors
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
if (FALSE) {
  shorten <- grep("-Wshorten-64-to-32", stderr, value = TRUE)

  
  line <- str_extract(shorten, "zstd.c:(\\d+):"      , group = 1) |> as.integer()
  pos  <- str_extract(shorten, "zstd.c:(\\d+):(\\d+)", group = 2) |> as.integer()
  
  # writeLines(zstd[line], "data-raw/wshorten.c")
  
  new_shorten <- readLines("data-raw/wshorten.c")
  
  zstd[line] <- new_shorten
  
  writeLines(zstd, "src/zstd/zstd.c")
}


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# 
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
signed <- grep("-Wsign-conversion", stderr, value = TRUE)
signed
line <- str_extract(signed, "zstd.c:(\\d+):"      , group = 1) |> as.integer()
pos  <- str_extract(signed, "zstd.c:(\\d+):(\\d+)", group = 2) |> as.integer()

from_type <- str_extract(signed, "signedness: '(.*?)'.*?to '(.*?)'", group = 1) 
to_type   <- str_extract(signed, "signedness: '(.*?)'.*?to '(.*?)'", group = 2) 

table(to_type)

idx <- to_type == 'unsigned int'
signed <- signed[idx]
line   <- line[idx]
pos    <- pos[idx]
from_type <- from_type[idx]
to_type   <- to_type[idx]



outbuf  <- sprintf("%6i %30s%17s    %s", line, from_type, to_type, zstd[line])
pointer <- vapply(pos, \(x) paste(c('#', rep(" ", x + 56), "^"), collapse = ""), character(1))

interleave <- function(x, y) {
  matrix(c(x, y), nrow = 2, byrow = TRUE) |> as.vector()
}

final <- interleave(outbuf, pointer)
cat(final, sep = "\n")

writeLines(final, "data-raw/sign-conversion.c")


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# slurp in fixes and apply to zstd.c
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
fix <- readLines("data-raw/sign-conversion.c")
fix <- fix[c(T, F)]
fix <- str_sub(fix, start = 59)

zstd <- ozstd
zstd[line] <- fix
writeLines(zstd, "src/zstd/zstd.c")



