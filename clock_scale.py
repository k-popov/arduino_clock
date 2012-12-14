#!/usr/bin/python -Qnew

# freq = 16e6 # document frequency
freq = 16008686.4234 # calculated frequency
prescalers = [1, 8, 32, 64, 128, 256, 1024]
# timer_max_value = 255
timer_max_value = 256*256 - 1
max_postscaler = 255
skew = 0.0

# setting worst defaults
best_skew = 1.0
best_prescaler = 1
best_counter_seed = 1
best_postscaler = max_postscaler

def better_result(cur_skew,
                  cur_prescaler,
                  cur_counter_seed,
                  cur_postscaler,
                  bst_skew,
                  bst_prescaler,
                  bst_counter_seed,
                  bst_postscaler
                  ):

    if cur_skew <= bst_skew:
        if cur_prescaler >= bst_prescaler:
            if cur_counter_seed >= bst_counter_seed:
                if cur_postscaler <= bst_postscaler:
# ############### alternative best logic #############
#    if cur_postscaler <= bst_postscaler:
#        if cur_counter_seed >= bst_counter_seed:
#            if cur_skew <= bst_skew:
#                if cur_prescaler >= bst_prescaler:
# ############### alternative best logic #############
                    # if new best is found
                    return True;
    # if this is not new best
    return False


for conter_seed in xrange(1,timer_max_value):
    for p in prescalers:
        postscaler = freq / p / (conter_seed + 1)
        if postscaler > max_postscaler: continue
        skew = postscaler - int(postscaler)
        if better_result(skew,
                         p,
                         conter_seed,
                         postscaler,
                         best_skew,
                         best_prescaler,
                         best_counter_seed,
                         best_postscaler
                        ):
            # new best result
            best_skew = skew
            best_prescaler = p
            best_counter_seed = conter_seed
            best_postscaler = postscaler
            # output this best
            print("skew:", skew, "prescaler: ", p,
                  "counter_seed: ", conter_seed,
                  "postscaler: ", postscaler)
print("finished")
