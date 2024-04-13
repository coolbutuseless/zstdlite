
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

orig <- serialize(df, NULL) |> length()
file <- tempfile()


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# saveRDS
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
compress_rds <- function(df, file, compress) {
  
  res <- bench::mark(saveRDS(df, file = file, compress = compress))
  
  res1 <- data.frame(
    type    = 'Compress (Serialize)',
    setting = as.character(compress),
    size    = file.size(file),
    time    = res$median |> as.numeric()
  )
  
  res <- bench::mark(readRDS(file))
  res2 <- data.frame(
    type    = 'Decompress (Unserialize)',
    setting = as.character(compress),
    size    = file.size(file),
    time    = res$median |> as.numeric()
  )
  
  rbind(res1, res2)
}

rds_df <- purrr::map_dfr(list(FALSE, 'gzip', 'bzip2', 'xz'), ~compress_rds(df, file, compress = .x))
rds_df$pkg <- 'saveRDS()/readRDS()'




#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# fst
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
compress_fst <- function(df, file, compress) {
  res <- bench::mark(write_fst(df, path = file, compress = compress))
  
  res1 <- data.frame(
    type    = 'Compress (Serialize)',
    setting = as.character(compress),
    size    = file.size(file),
    time    = res$median |> as.numeric()
  )
  
  res <- bench::mark(read_fst(file))
  res2 <- data.frame(
    type    = 'Decompress (Unserialize)',
    setting = as.character(compress),
    size    = file.size(file),
    time    = res$median |> as.numeric()
  )
  
  rbind(res1, res2)
}

fst_df <- purrr::map_dfr(seq(0, 100, 10), ~compress_fst(df, file, compress = .x))
fst_df$pkg <- 'fst'

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# zstdlite
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
compress_zstdlite <- function(df, file, compress) {
  cctx <- zstd_cctx(num_threads = 1, level = compress)
  res <- bench::mark(zstd_serialize(df, dst = file, cctx = cctx, use_file_streaming = FALSE))
  
  res1 <- data.frame(
    type    = 'Compress (Serialize)',
    setting = as.character(compress),
    size    = file.size(file),
    time    = res$median |> as.numeric()
  )
  
  dctx <- zstd_dctx()
  res <- bench::mark(zstd_unserialize(file, dctx = dctx))
  res2 <- data.frame(
    type    = 'Decompress (Unserialize)',
    setting = as.character(compress),
    size    = file.size(file),
    time    = res$median |> as.numeric()
  )
  
  rbind(res1, res2)
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
  
  res1 <- data.frame(
    type    = 'Compress (Serialize)',
    setting = as.character(compress),
    size    = file.size(file),
    time    = res$median |> as.numeric()
  )
  
  dctx <- zstd_dctx()
  res <- bench::mark(qread(file))
  res2 <- data.frame(
    type    = 'Decompress (Unserialize)',
    setting = as.character(compress),
    size    = file.size(file),
    time    = res$median |> as.numeric()
  )
  
  rbind(res1, res2)
}
qs_df <- purrr::map_dfr(seq(-5, 22, 3), ~compress_qs(df, file, compress = .x))
qs_df$pkg <- 'qs'



plot_df <- dplyr::bind_rows(rds_df, fst_df, zstdlite_df, qs_df)
plot_df <- dplyr::bind_rows(rds_df, zstdlite_df)

plot_df$speed <- orig / 1024 / 1024 / (as.numeric(plot_df$time))
plot_df$compression_ratio <- orig / plot_df$size

plot_df$Method <- plot_df$pkg


library(ggrepel)

ggplot(plot_df) +
  geom_path (aes(compression_ratio, speed, colour = Method), alpha = 0.3, show.legend = FALSE) + 
  geom_point(aes(compression_ratio, speed, colour = Method)) + 
  geom_text_repel(aes(compression_ratio, speed, colour = Method, label = setting), show.legend = FALSE) +
  theme_bw() + 
  expand_limits(x = 1) + 
  facet_wrap(~type) +
  labs(
    x = "Compression Ratio\n(bigger is better)",
    y = "Speed (MB/s)\n(bigger is better)",
    title = "Comparing `{zstdlite}` to `saveRDS()` for serializing R objects",
    subtitle = paste(
      "`{zstdlite}` can be much faster than `saveRDS(..., compress = 'gzip')`",
      "zstd decompression is fast & independent of compression settings",
      "zstd cannot quite reach maximum compression of `saveRDS(..., compress = 'xz')`",
      sep = "\n"
    )
  ) +
  theme(
    legend.position = 'bottom'
  ) + 
  scale_x_continuous(breaks = scales::pretty_breaks()) + 
  scale_y_log10()


ggsave("./man/figures/comparison.png", width = 1200, height = 800, units = 'px', dpi = 120)






