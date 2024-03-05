
library(zstdlite)
library(fst)
library(bench)
library(ggplot2)
library(purrr)
library(qs)



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Benchmark dataset from {fst}
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
nr_of_rows <- 1e5

df <- data.frame(
  Logical = sample(c(TRUE, FALSE, NA), prob = c(0.85, 0.1, 0.05), nr_of_rows, replace = TRUE),
  Integer = sample(nr_of_rows, replace = TRUE),
  Real = sample(sample(nr_of_rows, 20) / 100, nr_of_rows, replace = TRUE),
  Factor = as.factor(sample(labels(UScitiesD), nr_of_rows, replace = TRUE))
)


file <- tempfile()


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# saveRDS
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
compress_rds <- function(df, file, compress) {
  
  res <- bench::mark(saveRDS(df, file = file, compress = compress))
  
  data.frame(
    setting = as.character(compress),
    size    = file.size(file),
    time    = res$median |> as.numeric()
  )
}

rds_df <- purrr::map_dfr(list(FALSE, 'gzip', 'bzip2', 'xz'), ~compress_rds(df, file, compress = .x))
rds_df$pkg <- 'rds'

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# fst
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
compress_fst <- function(df, file, compress) {
  res <- bench::mark(write_fst(df, path = file, compress = compress))
  
  data.frame(
    setting = as.character(compress),
    size    = file.size(file),
    time    = res$median |> as.numeric()
  )
}

fst_df <- purrr::map_dfr(seq(0, 100, 10), ~compress_fst(df, file, compress = .x))
fst_df$pkg <- 'fst'

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# zstdlite
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
compress_zstdlite <- function(df, file, compress) {
  cctx <- zstd_cctx(num_threads = 1, level = compress)
  res <- bench::mark(zstd_serialize(df, file = file, cctx = cctx, use_file_streaming = FALSE))
  
  data.frame(
    setting = as.character(compress),
    size    = file.size(file),
    time    = res$median |> as.numeric()
  )
}


zstdlite_df <- purrr::map_dfr(seq(-5, 22, 3), ~compress_zstdlite(df, file, compress = .x))
zstdlite_df$pkg <- 'zstdlite'


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# {qs}
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
compress_qs <- function(df, file, compress) {
  res <- bench::mark(
    qsave(df, file = file, preset = 'custom', compress_level = compress, check_hash = FALSE)
  )
  
  data.frame(
    setting = as.character(compress),
    size    = file.size(file),
    time    = res$median |> as.numeric()
  )
}
qs_df <- purrr::map_dfr(seq(-5, 22, 3), ~compress_qs(df, file, compress = .x))
qs_df$pkg <- 'qs'



plot_df <- dplyr::bind_rows(rds_df, fst_df, zstdlite_df, qs_df)


ggplot(plot_df) +
  geom_path(aes(size, time, colour = pkg)) + 
  theme_bw() + 
  expand_limits(x = 0)








