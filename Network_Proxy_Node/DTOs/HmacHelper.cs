using System;
using System.Security.Cryptography;
using System.Text;

public static class HmacHelper
{
    // Trả về chữ ký dạng hex chữ thường (GIẢ ĐỊNH TV1 dùng hex)
    public static string ComputeHmacSha256Hex(string payload, string secret)
    {
        byte[] key = Encoding.UTF8.GetBytes(secret);
        byte[] data = Encoding.UTF8.GetBytes(payload);

        using var hmac = new HMACSHA256(key);
        byte[] hash = hmac.ComputeHash(data);

        return Convert.ToHexString(hash).ToLowerInvariant();
    }

    // So sánh thời gian cố định, chống timing attack
    public static bool SignatureEquals(string a, string b)
    {
        byte[] x = Encoding.UTF8.GetBytes(a.ToLowerInvariant());
        byte[] y = Encoding.UTF8.GetBytes(b.ToLowerInvariant());
        return CryptographicOperations.FixedTimeEquals(x, y);
    }

    // Timestamp tính bằng Unix seconds, lệch tối đa maxSkewSeconds
    public static bool IsTimestampValid(long clientTimestamp, int maxSkewSeconds)
    {
        long now = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
        return Math.Abs(now - clientTimestamp) <= maxSkewSeconds;
    }
}