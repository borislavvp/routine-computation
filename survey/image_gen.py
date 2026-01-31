prompt_template = """
Create a symbolic visual illustration of a user and their ideal AI assistant, based on the following preferences and lifestyle details:

User Profile:
- Age: {age}
- Daily routine structure: {routine_consistency}
- Planning habits: {planning_frequency}
- Reflection habits: {reflection_frequency}
- Disruption tolerance: {routine_disruption}
- Current assistant usage: {chatbot_usage}

User Preferences for Assistant Behavior:
- Most valued assistant abilities: {top_abilities}
- Assistant personality traits: {personality_traits}
- Privacy preferences (audio): {audio_privacy}
- Privacy preferences (camera): {camera_privacy}
- Trust factors: {trust_factors}

Scene & Environment:
Design a stylized visual scene in the style of **{visual_style}**. The user is situated in a daily context that reflects their lifestyle—either structured or spontaneous, indoors or outdoors, peaceful or dynamic. The AI assistant should appear as a distinct yet integrated companion, represented visually (e.g., humanoid, abstract aura, floating device, etc.).

Assistant Interaction:
Depict the assistant engaging with the user in a way that reflects their chosen behaviors:
- If proactive, show the assistant acting first, offering suggestions or adjusting the scene.
- If focused on reminders or planning, depict calendar tools, clocks, or schedules shared between them.
- If health-related assistance is preferred, use physical cues like wellness monitors, water bottles, or yoga mats.
- For emotionally sensitive or private assistants, use soft lighting, distance, transparency, or protective symbols (e.g., shields, bubbles).

Visualizing Assistant Personality:
Incorporate assistant personality traits through design cues:
- Empathy: warm lighting, soft edges, floral motifs
- Calmness: cool tones, minimal movement, sitting posture
- High initiative: strong posture, bright highlights, leading motion
- Logical or task-focused: geometric shapes, charts, checklists

Privacy & Trust:
Visually hint at privacy control preferences: microphone icons muted, camera shutters closed, lock symbols, transparent data bubbles, etc., based on:
- Audio: {audio_privacy}
- Camera: {camera_privacy}
- Data sharing: {data_monetization_opinion}
- Emergency autonomy: {emergency_intervention_opinion}

Final Instructions:
- Ensure the assistant looks emotionally or symbolically connected to the user, depending on their responses. Make the image feel poetic and personal, not clinical or futuristic unless high-tech affinity is suggested. The assistant should feel helpful, adaptive, and trustworthy.
- Use the attached {visual_style} image as aesthetic throughout. Keep composition similar, clean and storytelling-driven.
"""


if __name__ == "__main__":
    # Example usage
    example_data = {
        "age": "30",
        "routine_consistency": "highly structured",
        "planning_frequency": "daily",
        "reflection_frequency": "weekly",
        "routine_disruption": "low tolerance",
        "chatbot_usage": "frequent",
        "top_abilities": "proactive reminders, health tracking, emotional support",
        "personality_traits": "empathetic, calm, logical",
        "audio_privacy": "high",
        "camera_privacy": "medium",
        "trust_factors": "transparency, reliability, data security",
        "visual_style": "minimalist"
    }
    
    prompt = prompt_template.format(**example_data)

    print(prompt)