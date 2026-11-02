"""Render firmware signaling states to GIF. Requires Pillow (pip install Pillow)."""
from pathlib import Path
import re

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]


def source(name):
    text = (ROOT / 'main' / name).read_text(encoding='utf-8')
    return re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)


def main():
    patterns_source = source('patterns.c')
    palette = {
        name: tuple(map(int, rgb.split(',')))
        for name, rgb in re.findall(r'\[(CLR_\w+)\]\s*=\s*\{([\d\s,]+)\}', patterns_source)
    }
    macros = dict(re.findall(r'#define\s+(\w+)\s+(CLR_\w+)', patterns_source))
    patterns = {}
    for name, body in re.findall(r'const pattern_t (PAT_\w+)\s*=\s*\{(.*?)\n\};', patterns_source, re.S):
        pixels = re.search(r'\.px\s*=\s*\{(.*)\}', body, re.S).group(1)
        tokens = re.findall(r'\b[A-Z][A-Z0-9]*\b', pixels)
        if len(tokens) != 64:
            raise ValueError(f'{name}: expected 64 pixels, got {len(tokens)}')
        patterns[name] = [palette[macros[token]] for token in tokens]
    timings = dict(re.findall(r'#define\s+(\w+)\s+(\d+)', source('config.h')))
    animator = source('led_animator.c')
    animations = {}
    for name, body in re.findall(r'static const frame_t (s_\w+)\[\]\s*=\s*\{(.*?)\};', animator, re.S):
        animations[name] = [
            (pattern, int(timings.get(duration, duration)))
            for pattern, duration in re.findall(r'\{\s*&?(PAT_\w+|NULL)\s*,\s*(\w+)\s*\}', body)
        ]
    states = re.findall(r'\[ROBOT_STATE_(\w+)\]\s*=\s*\{\s*(s_\w+)', animator)
    declared = re.findall(r'^\s*ROBOT_STATE_(\w+)', source('robot_state.h'), re.M)
    assert {state for state, _ in states} == set(declared) - {'COUNT'}
    font = ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf', 23)
    small = ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf', 15)

    def render(pattern, state, number):
        image = Image.new('RGB', (480, 460), 'white')
        draw = ImageDraw.Draw(image)
        draw.text((240, 23), state.replace('_', ' '), font=font, fill='#10151e', anchor='mt')
        draw.rounded_rectangle((66, 70, 414, 418), radius=16, fill='#050709', outline='#303b4c', width=2)
        pixels = patterns.get(pattern, [(0, 0, 0)] * 64)
        for i, rgb in enumerate(pixels):
            x, y = 84 + (i % 8) * 40, 88 + (i // 8) * 40
            draw.rounded_rectangle((x, y, x + 31, y + 31), radius=7,
                                   fill=rgb if any(rgb) else '#171c24')
        draw.text((240, 433), f'{number:02d} / {len(states):02d}  |  8 x 8 LED panel',
                  font=small, fill='#526075', anchor='mt')
        return image

    output = ROOT / 'docs' / 'led-signals'
    output.mkdir(parents=True, exist_ok=True)
    all_frames, all_durations = [], []
    for number, (state, animation) in enumerate(states, 1):
        sequence = animations[animation]
        # Repeat short cycles until each state has been visible for >= 2 seconds.
        cycle_ms = sum(ms or 2000 for _, ms in sequence)
        repeats = max(1, (2000 + cycle_ms - 1) // cycle_ms)
        frames, durations = [], []
        for _ in range(repeats):
            for pattern, ms in sequence:
                frames.append(render(pattern, state, number))
                # GIF durations use 10 ms units; play at approximately 2x speed.
                durations.append(max(10, round((ms or 2000) / 20) * 10))
        frames[0].save(output / f'{state.lower()}.gif', save_all=True,
                       append_images=frames[1:], duration=durations, loop=0, disposal=2)
        all_frames.extend(frames)
        all_durations.extend(durations)
    target = output / 'all-signals.gif'
    all_frames[0].save(target, save_all=True, append_images=all_frames[1:],
                       duration=all_durations, loop=0, disposal=2)
    with Image.open(target) as gif:
        actual_ms = 0
        for index in range(gif.n_frames):
            gif.seek(index)
            actual_ms += gif.info['duration']
        assert actual_ms == sum(all_durations)
        assert gif.info['loop'] == 0
        print(f'{target}: {len(states)} states, {gif.n_frames} frames, {actual_ms / 1000:.2f}s')


if __name__ == '__main__':
    main()
