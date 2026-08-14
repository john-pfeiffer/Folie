// Offline DSP render tests. Plain assert-style checks, no framework.
// Each test renders audio headlessly and exits non-zero on failure.

#include <juce_dsp/juce_dsp.h>

#include <cstdio>

namespace
{
int failures = 0;

void check (bool condition, const char* what)
{
    if (condition)
        std::printf ("PASS  %s\n", what);
    else
    {
        std::printf ("FAIL  %s\n", what);
        ++failures;
    }
}
} // namespace

int main()
{
    // Scaffold smoke test: JUCE dsp module links and runs headless.
    juce::dsp::FFT fft (10);
    check (fft.getSize() == 1024, "juce::dsp links and initialises");

    std::printf (failures == 0 ? "All tests passed.\n" : "%d test(s) FAILED.\n", failures);
    return failures == 0 ? 0 : 1;
}
