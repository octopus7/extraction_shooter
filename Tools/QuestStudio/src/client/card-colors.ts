// QuestStudio presentation only. Missing or unrecognized values keep the original gray.
export const CARD_COLORS = [
  { id: 'default', background: '#202630', labelKey: 'cardColor.default' },
  { id: 'blue', background: '#28313b', labelKey: 'cardColor.blue' },
  { id: 'green', background: '#29342f', labelKey: 'cardColor.green' },
  { id: 'sand', background: '#353229', labelKey: 'cardColor.sand' },
  { id: 'rose', background: '#362c31', labelKey: 'cardColor.rose' },
  { id: 'purple', background: '#312d3b', labelKey: 'cardColor.purple' },
] as const;

export function resolveCardColor(value: unknown) {
  return CARD_COLORS.find(color => color.id === value) ?? CARD_COLORS[0];
}
